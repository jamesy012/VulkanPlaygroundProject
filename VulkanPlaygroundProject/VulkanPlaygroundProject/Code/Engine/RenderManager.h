#pragma once

class Model;
class Transform;
class BufferStorageUniform;

struct RenderingBufferOUT;
class BufferVertex;
class BufferIndex;

class RenderManager {
	//Singleton
private:
	static RenderManager* _RenderManager;
public:
	static inline RenderManager* Get() { return _RenderManager; };

	//Implementation
public:
	RenderManager();
	~RenderManager();

	void StartUp();

	void AddModel(Model* aModel);
	void AddObject(Model* aModel, Transform* aTransform);

	void Render(VkCommandBuffer aBuffer);
private:
	BufferStorageUniform* mDrawDataBuffer;
	RenderingBufferOUT* mDrawDataBufferMappedData;

	BufferVertex* mVertexBuffer;
	BufferVertex* mVertexInstanceBuffer;
	BufferIndex* mIndexBuffer;
	uint32_t mVertexCount = 0;
	uint32_t mIndexCount = 0;


	RenderingBufferOUT* GetDrawDataBuffer(uint32_t aIndex = 0);
};

