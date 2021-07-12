#include "stdafx.h"
#include "Terrain.h"

void Terrain::Create(std::string aTexturePath) {
   CreateMesh();
}

void Terrain::Destroy() {
   mVertexBuffer.Destroy();
}

void Terrain::Render(VkCommandBuffer aBuffer) {
   mVertexBuffer.Bind(aBuffer);
   vkCmdDraw(aBuffer, static_cast<uint32_t>(mVertexBuffer.GetSize()), 1, 0, 0);
}

void Terrain::CreateMesh() {
   mVertices.resize(mWidth * mHeight * 6u);
   for (uint32_t w = 0; w < mWidth; w++) {
      for (uint32_t h = 0; h < mHeight; h++) {
         glm::vec3 tl = glm::vec3((w+0) * mTileSize, 0, (h+1) * mTileSize);
         glm::vec3 tr = glm::vec3((w+1) * mTileSize, 0, (h+1) * mTileSize);
         glm::vec3 br = glm::vec3((w+1) * mTileSize, 0, (h+0) * mTileSize);
         glm::vec3 bl = glm::vec3((w+0) * mTileSize, 0, (h+0) * mTileSize);
         glm::vec2 uvtl = glm::vec2((float)(w+0) / mWidth, (float)(h+1) / mHeight);
         glm::vec2 uvtr = glm::vec2((float)(w+1) / mWidth, (float)(h+1) / mHeight);
         glm::vec2 uvbr = glm::vec2((float)(w+1) / mWidth, (float)(h+0) / mHeight);
         glm::vec2 uvbl = glm::vec2((float)(w+0) / mWidth, (float)(h+0) / mHeight);
         uint64_t index = (h * mWidth + w) *6;
         mVertices[index+0].pos = tl;
         mVertices[index+0].texCoord = uvtl;
         mVertices[index+1].pos = tr;
         mVertices[index+1].texCoord = uvtr;
         mVertices[index+2].pos = br; 
         mVertices[index+2].texCoord = uvbr;
         mVertices[index+3].pos = br;
         mVertices[index+3].texCoord = uvbr;
         mVertices[index+4].pos = bl;
         mVertices[index+4].texCoord = uvbl;
         mVertices[index+5].pos = tl;
         mVertices[index+5].texCoord = uvtl;
      }
   }

   mVertexBuffer.Create(mVertices.size() * sizeof(Vertex));
   //mIndexBuffer.Create(mIndices.size() * sizeof(uint32_t));

   BufferStaging staging;
   staging.Create(std::max(mVertexBuffer.GetAllocatedSize(), mIndexBuffer.GetAllocatedSize()));
   void* data;
   staging.Map(&data);
   memcpy(data, mVertices.data(), mVertexBuffer.GetSize());
   mVertexBuffer.CopyFrom(&staging);
   //memcpy(data, mIndices.data(), mIndexBuffer.GetSize());
   //mIndexBuffer.CopyFrom(&staging);
   staging.UnMap();
   staging.Destroy();

}
