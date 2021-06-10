#pragma once

#include "Vertex.h"
#include "Buffer.h"

#include "Image.h"

struct aiScene;
struct aiNode;

struct DescriptorUBO {
public:
   DescriptorUBO(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, BufferRingUniform* aSceneBuffer, BufferRingUniform* aObjectBuffer) {
      mCommandBuffer = aCommandBuffer;
      mPipelineLayout = aPipelineLayout;
      mSceneBuffer = aSceneBuffer;
      mObjectBuffer = aObjectBuffer;

      mDescriptorSet = aSceneBuffer->GetDescriptorSet();

      mObjectBuffer->Get();
   }

   ~DescriptorUBO() {
      mObjectBuffer->Return();
   }

   void UpdateObjectAndBind(void* aData) {
      void* objectUbo;
      mObjectBuffer->Get((void**)&objectUbo);
      memcpy(objectUbo, aData, mObjectBuffer->GetStructSize());
      mObjectBuffer->Return();
      BindDescriptorSet();
   }

   void BindDescriptorSet() {
      uint32_t descriptorSetOffsets[] = { mSceneBuffer->GetCurrentOffset(), mObjectBuffer->GetCurrentOffset() };
      vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet, 2, descriptorSetOffsets);
   }

   BufferRingUniform* mSceneBuffer;
   BufferRingUniform* mObjectBuffer;
   VkCommandBuffer mCommandBuffer;
   VkPipelineLayout mPipelineLayout;
private:
   VkDescriptorSet mDescriptorSet;
};


class Model {
   //~~~~~~~~~~ NODE/MESH
   struct Mesh {
      Mesh() {};
      int mMaterialID;
      int mCount;
      int mStartIndex;
      int mStartVertex;
   };

   struct Node {
      Node() {
         mRenderMatrix = glm::identity<glm::mat4>();
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

   enum class ImageType {
      DIFFUSE,
      NORMAL
   };

   struct Material {
      std::vector<Image*> mDiffuse;
      std::vector<Image*> mNormal;

      VkDescriptorSet mDescriptorSet;
   };

   struct ImageLoader {
      std::string mPath;
      std::vector<Material*> mMaterialRef;
      ImageType mType;
   };

   std::vector<ImageLoader> mImageLoader;
   std::vector<Image> mImages;//break out to image manager class?
   std::vector<Material> mMaterials;

public:
   bool LoadModel(std::string aPath, VkDescriptorSetLayout aMaterialDescriptorSet);
   void Render(DescriptorUBO* aRenderDescriptor, RenderMode aRenderMode);

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
   void Render(DescriptorUBO* aRenderDescriptor, RenderMode aRenderMode, Node* aNode, glm::mat4 aMatrix);
   void ProcessMeshs(const aiScene* aScene);
   void ProcessMesh(const aiScene* aScene, const aiNode* aNode, Node* aParent);
   void ProcessMaterials(const aiScene* aScene);
   void LoadImages();

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
