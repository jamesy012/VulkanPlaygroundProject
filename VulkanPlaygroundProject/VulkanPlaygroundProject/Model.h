#pragma once

#include "Vertex.h"
#include "Buffer.h"

#include "Image.h"
#include "Transform.h"

struct aiScene;
struct aiNode;

struct DescriptorUBO {
public:
   DescriptorUBO(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, BufferRingUniform* aObjectBuffer) {
      mCommandBuffer = aCommandBuffer;
      mPipelineLayout = aPipelineLayout;
      mObjectBuffer = aObjectBuffer;

      mDescriptorSet = aObjectBuffer->GetDescriptorSet();

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
      uint32_t descriptorSetOffsets[] = { mObjectBuffer->GetCurrentOffset() };
      vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1, 1, &mDescriptorSet, 1, descriptorSetOffsets);
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
      int mMaterialID = -1;
      int mCount = 0;
      int mStartIndex = 0;
      int mStartVertex = 0;
   };

   struct Node {
      Node() {
      }

      glm::mat4 GetMatrixWithParents() {
         return mTransform.GetGlobalMatrix();
      }

      glm::mat4 GetMatrix() {
         return mTransform.GetLocalMatrix();
      }

      std::string mName;

      Transform mTransform;
      Node* mParent = nullptr;
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
      ASSERT_RET(mBase);
      mBase->mTransform.SetPosition(aPos);
   }
   void SetRotation(glm::vec3 aRot) {
      ASSERT_RET(mBase);
      mBase->mTransform.SetRotation(aRot);
   }
   void SetScale(float aScale) {
      ASSERT_RET(mBase);
      mBase->mTransform.SetScale(aScale);
   }
   void SetScale(glm::vec3 aScale) {
      ASSERT_RET(mBase);
      mBase->mTransform.SetScale(aScale);
   }
   glm::vec3 GetPosition() {
      ASSERT_RET_VALUE(mBase, glm::vec3(0));
      return mBase->mTransform.GetPostion();
   }
   glm::vec3 GetRotation() {
      ASSERT_RET_VALUE(mBase, glm::vec3(0));
      return mBase->mTransform.GetRotation();
   }
   glm::vec3 GetScale() {
      ASSERT_RET_VALUE(mBase, glm::vec3(1));
      return mBase->mTransform.GetScale();
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
};
