#pragma once

#include "Vertex.h"

class Terrain {
public:
   void Create(std::string aTexturePath);
   void Destroy();
   void Render(VkCommandBuffer aBuffer);
private:
   void CreateMesh();

   uint32_t mWidth = 64;
   uint32_t mHeight = 64;
   float mTileSize = 4.0f;

   //merge with Model?
   std::vector<Vertex> mVertices;
   std::vector<uint32_t> mIndices;
   BufferVertex mVertexBuffer;
   BufferIndex mIndexBuffer;
};

