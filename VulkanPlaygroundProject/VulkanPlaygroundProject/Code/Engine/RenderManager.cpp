#include "stdafx.h"
#include "RenderManager.h"

#include "Model.h"

RenderManager* RenderManager::_RenderManager = nullptr;

struct RenderingBufferOUT
{
   uint32_t    indexCount;
   uint32_t    instanceCount;
   uint32_t    firstIndex;
   int32_t     vertexOffset;
   uint32_t    firstInstance;
};

void RenderManager::StartUp()
{
}

void RenderManager::AddModel( Model* aModel )
{
   for ( int i = 0; i < aModel->GetNumNodes(); i++ )
   {
      Model::Node* node = aModel->GetNode( i );
      for ( int q = 0; q < node->mMesh.size(); q++ )
      {
         Model::Mesh* mesh = &node->mMesh[q];
         RenderingBufferOUT* bufferPtr = nullptr;//ptr to memory
         if ( bufferPtr )
         {
            memset( bufferPtr, 0, sizeof( RenderingBufferOUT ) );
            bufferPtr->firstIndex = mesh->mStartIndex;
            bufferPtr->indexCount = mesh->mCount;
            //bufferPtr->instanceCount = 1; //incremented via AddObject
         }
      }
   }
}
