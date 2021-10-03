#include "stdafx.h"
#include "RenderManager.h"

#include "Model.h"
#include "Buffer.h"
#include "Vertex.h"

RenderManager* RenderManager::_RenderManager = nullptr;

#define MAX_MODEL_COUNT 128

static uint32_t MappedModelDataIndex = 0;//temp


struct RenderingBufferOUT //could replace with VkDrawIndexedIndirectCommand
{
	uint32_t    indexCount;
	uint32_t    instanceCount;
	uint32_t    firstIndex;
	int32_t     vertexOffset;
	uint32_t    firstInstance;
};

struct InstanceData {
	uint32_t    indexCount;
	uint32_t    instanceCount;
	uint32_t    firstIndex;
	int32_t     vertexOffset;
	uint32_t    firstInstance;
};

RenderManager::RenderManager() : mDrawDataBuffer(nullptr), mDrawDataBufferMappedData(nullptr) {
	ASSERT_IF(_RenderManager == nullptr);
	_RenderManager = this;
	mDrawDataBuffer = new BufferStorageUniform();

	mVertexBuffer = new BufferVertex();
	mVertexInstanceBuffer = new BufferVertex();
	mIndexBuffer = new BufferIndex();
}

RenderManager::~RenderManager() {
	ASSERT_IF(_RenderManager != nullptr);
	mDrawDataBuffer->UnMap();
	mDrawDataBufferMappedData = nullptr;
	mDrawDataBuffer->Destroy();
	delete mDrawDataBuffer;
	mDrawDataBuffer = nullptr;

	mVertexBuffer->Destroy();
	mVertexInstanceBuffer->Destroy();
	mIndexBuffer->Destroy();
	mVertexBuffer = nullptr;
	mVertexInstanceBuffer = nullptr;
	mIndexBuffer = nullptr;

	_RenderManager = nullptr;
}

void RenderManager::StartUp() {
	mDrawDataBuffer->Create(MAX_MODEL_COUNT, sizeof(RenderingBufferOUT), VK_NULL_HANDLE, 0, false);
	mDrawDataBuffer->Map((void**)&mDrawDataBufferMappedData);
	memset(mDrawDataBufferMappedData, 0, sizeof(RenderingBufferOUT) * MAX_MODEL_COUNT);
	mDrawDataBuffer->SetName("Render Manager Draw Data Buffer");

	mVertexBuffer->Create(1024 * 1024 * 1024);
	mIndexBuffer->Create(1024 * 1024 * 1024);
	mVertexInstanceBuffer->Create(1024 * sizeof(VertexInstanceInstance));
	mVertexBuffer->SetName("Render Manager Vertex Buffer");
	mIndexBuffer->SetName("Render Manager Index Buffer");
	mVertexInstanceBuffer->SetName("Render Manager Vertex Instance Buffer");

	{
		BufferStaging staging;
		staging.Create(1024);
		VertexInstanceInstance* data;
		staging.Map((void**)&data);
		data[0].instancePos = glm::vec3(0);
		data[1].instancePos = glm::vec3(0,0,90);
		staging.UnMap();

		mVertexInstanceBuffer->CopyFrom(&staging);

		staging.Destroy();
	}
}

void RenderManager::AddModel(Model* aModel) {
	ASSERT_IF(mVertexBuffer->GetSize() > mVertexCount + aModel->mVertices.size());
	ASSERT_IF(mIndexBuffer->GetSize() > mIndexCount + aModel->mIndices.size());

	uint32_t vertexCount = mVertexCount;
	uint32_t indexCount = mIndexCount;
	for(int i = 0; i < aModel->GetNumNodes(); i++) {
		Model::Node* node = aModel->GetNode(i);
		for(int q = 0; q < node->mMesh.size(); q++) {
			Model::Mesh* mesh = &node->mMesh[q];
			RenderingBufferOUT* bufferPtr = GetDrawDataBuffer(MappedModelDataIndex++);//ptr to memory
			if(bufferPtr) {
				memset(bufferPtr, 0, sizeof(RenderingBufferOUT));
				bufferPtr->firstIndex = indexCount + mesh->mStartIndex;
				bufferPtr->indexCount = mesh->mCount;
				bufferPtr->instanceCount = 2; //incremented via AddObject?
			}
		}
	}

	mVertexCount += aModel->mVertices.size();
	mIndexCount += aModel->mIndices.size();
	//mVertexBuffer->CopyFrom(&aModel->mVertexBuffer);
	//mIndexBuffer->CopyFrom(&aModel->mIndexBuffer);
	{
		BufferStaging staging;
		staging.Create(std::max(aModel->mVertexBuffer.GetAllocatedSize(), aModel->mIndexBuffer.GetAllocatedSize()));
		void* data;
		staging.Map(&data);
		memcpy(data, aModel->mVertices.data(), aModel->mVertexBuffer.GetSize());
		mVertexBuffer->CopyFrom(&staging);
		memcpy(data, aModel->mIndices.data(), aModel->mIndexBuffer.GetSize());
		mIndexBuffer->CopyFrom(&staging);
		staging.UnMap();
		staging.Destroy();
	}
}

void RenderManager::AddObject(Model* aModel, Transform* aTransform) {
}

void RenderManager::Render(VkCommandBuffer aBuffer) {
	mVertexBuffer->Bind(aBuffer, 0);
	mIndexBuffer->Bind(aBuffer);
	mVertexInstanceBuffer->Bind(aBuffer, 1);

	vkCmdDrawIndexedIndirect(aBuffer, mDrawDataBuffer->GetBuffer(), 0, MappedModelDataIndex, sizeof(VkDrawIndexedIndirectCommand));
}

RenderingBufferOUT* RenderManager::GetDrawDataBuffer(uint32_t aIndex) {
	ASSERT_IF(mDrawDataBufferMappedData != nullptr);
	ASSERT_IF(MAX_MODEL_COUNT > aIndex);
	if(mDrawDataBufferMappedData == nullptr) {
		return nullptr;
	}
	return &mDrawDataBufferMappedData[aIndex];
}
