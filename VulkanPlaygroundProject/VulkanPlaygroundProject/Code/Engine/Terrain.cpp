#include "stdafx.h"
#include "Terrain.h"

void Terrain::Create(std::string aTexturePath) {
   mWidth = mHeight = 256;
   mTileSize = 1;
   CreateMesh();
}

void Terrain::Destroy() {
   mVertexBuffer.Destroy();
   mIndexBuffer.Destroy();
}

void Terrain::Render(VkCommandBuffer aCommandBuffer, VkPipelineLayout aLayout, BufferRingUniform* aBuffer, VkDescriptorSet aDescriptorSet) {
   mVertexBuffer.Bind(aCommandBuffer);
   mIndexBuffer.Bind(aCommandBuffer);

   DescriptorUBO des = DescriptorUBO(aCommandBuffer, aLayout, aBuffer, aDescriptorSet);
   ObjectUBO ubo;
   ubo.mModel = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-32, -4, -32) * 4.0f);
   des.UpdateObjectAndBind(&ubo);

   vkCmdDrawIndexed(aCommandBuffer, static_cast<uint32_t>(mIndices.size()), 1, 0, 0, 0);
}

void Terrain::CreateMesh() {
   mVertices.resize(mWidth * mHeight);
   mIndices.resize(mWidth * mHeight * 6);
   for (uint32_t w = 0; w < mWidth; w++) {
      for (uint32_t h = 0; h < mHeight; h++) {
         uint64_t index = (h * mWidth + w);
         mVertices[index].pos = glm::vec3(w* mTileSize, 0, h * mTileSize);
         mVertices[index].texCoord = glm::vec2((float)w / mWidth, (float)h / mHeight);
      }
   }

   for (uint32_t w = 0; w < mWidth-1; w++) {
      for (uint32_t h = 0; h < mHeight-1; h++) {
         uint64_t index = (h * mWidth + w) * 6u;
         int tl = (w + h * mWidth);
         int tr = tl+1;
         int bl = tl+mWidth;
         int br = bl+1;
         mIndices[index + 0] = br;
         mIndices[index + 1] = tr;
         mIndices[index + 2] = tl;
         mIndices[index + 3] = tl;
         mIndices[index + 4] = bl;
         mIndices[index + 5] = br;
      }
   }

   mVertexBuffer.Create(mVertices.size() * sizeof(Vertex));
   mIndexBuffer.Create(mIndices.size() * sizeof(uint32_t));

   BufferStaging staging;
   staging.Create(std::max(mVertexBuffer.GetAllocatedSize(), mIndexBuffer.GetAllocatedSize()));
   void* data;
   staging.Map(&data);
   memcpy(data, mVertices.data(), mVertexBuffer.GetSize());
   mVertexBuffer.CopyFrom(&staging);
   memcpy(data, mIndices.data(), mIndexBuffer.GetSize());
   mIndexBuffer.CopyFrom(&staging);
   staging.UnMap();
   staging.Destroy();

}
