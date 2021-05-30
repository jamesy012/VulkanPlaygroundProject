#pragma once

#include "Vertex.h"
#include "Buffer.h"

struct aiScene;
struct aiNode;

class Model {
   //~~~~~~~~~~ NODE/MESH
   struct Mesh {
      int mMaterialID;
      int mCount;
      int mStartIndex;
      int mStartVertex;
   };

   struct Node {
      Node() {
         mRenderMatrix = /*m_Matrix =*/ glm::identity<glm::mat4>();
      }


      glm::mat4 GetMatrixWithParents() {
         Node* parent = mParent;
         glm::mat4 objectParentMatrix = GetMatrix();
         while (parent != nullptr) {
            objectParentMatrix = parent->GetMatrix() * objectParentMatrix;
            parent = parent->mParent;
         }
         return objectParentMatrix;
      }

      glm::mat4 GetMatrix() {
         return mRenderMatrix;
      }

      Node* mParent = nullptr;
      glm::mat4 mRenderMatrix;
      std::string mName;
      std::vector<Node*> mChildren;
      std::vector<Mesh> mMesh;
   };

public:
   bool LoadModel(std::string aPath);
   void Render(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, RenderMode aRenderMode);

   void SetPosition(glm::vec3 aPos) {
      mDirty = true;
      mPosition = aPos;
   }

   void SetRotation(glm::vec3 aRot) {
      mDirty = true;
      mRotation = aRot;
   }

   void SetScale(float aScale) {
      mDirty = true;
      mScale = glm::vec3(aScale);
   }
   void SetScale(glm::vec3 aScale) {
      mDirty = true;
      mScale = aScale;
   }
   glm::vec3 GetPosition() {
      return mPosition;
   }
   glm::vec3 GetRotation() {
      return mRotation;
   }
   glm::vec3 GetScale() {
      return mScale;
   }

   void SetRenderMode(unsigned int aMode) {
      mRenderModes = aMode;
   }
   void SetRenderMode(RenderMode aMode, bool aState) {
      if (aState) {
         AddRenderMode(aMode);
      } else {
         RemoveRenderMode(aMode);
      }
   }
   bool IsRenderMode(unsigned int aMode) {
      return mRenderModes & aMode;
   }

   void RemoveRenderMode(unsigned int aMode) {
      mRenderModes = mRenderModes & ~aMode;
   }
   void AddRenderMode(unsigned int aMode) {
      mRenderModes |= aMode;
   }

   void Destroy();

   ~Model() {
      if (mVertices.size() != 0) {
         Destroy();
      }
   }

private:
   void Render(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, RenderMode aRenderMode, Node* aNode, glm::mat4 aMatrix);
   void ProcessMeshs(const aiScene* aScene);
   void ProcessMesh(const aiScene* aScene, const aiNode* aNode, Node* aParent);

   std::vector<Mesh*> mMeshs;
   Node* mBase;
   std::vector<Node*> mNodes;

   unsigned int mRenderModes = RenderMode::ALL;

   std::string mName;
   std::string mPath;
   std::string mTexturePath;


   std::vector<Vertex> mVertices;
   std::vector<uint32_t> mIndices;

   BufferVertex mVertexBuffer;
   BufferIndex mIndexBuffer;

   bool mDirty = true;
   glm::vec3 mScale = glm::vec3(1.0f);
   glm::vec3 mPosition = glm::vec3(0);
   glm::vec3 mRotation = glm::vec3(0);
};
