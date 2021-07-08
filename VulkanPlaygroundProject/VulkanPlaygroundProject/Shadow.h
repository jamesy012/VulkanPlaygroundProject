#pragma once

class RenderPass;
class ShadowManager {
protected:
   static ShadowManager* mInstance;
public:
   static void Create();
   static void Destroy();
   static const RenderPass* GetRenderPass() {
      ASSERT_IF(mInstance != nullptr);
      return mInstance->mRenderPass;
   }

private:
   RenderPass* mRenderPass;
};

class Shadow {
public:
   void Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout);
   void Destroy();

   void StartRenderPass(VkCommandBuffer aBuffer);
   void EndRenderPass(VkCommandBuffer aBuffer);



   const class Image* GetImage() const {
      return mDepthImage;
   }

   void SetName(std::string aName);

   //Transform mTransform;
private:
   VkExtent2D mSize;
   VkDescriptorSet mShadowSet = VK_NULL_HANDLE;

   class Image* mDepthImage;
   class Framebuffer* mFramebuffer;
};

class ShadowDirectional : public Shadow {
};

class ShadowDirectionalCascade {
public:
   void Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout, uint32_t aNumCascades = 3);
   void Destroy();

   void StartRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex);
   void EndRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex);

   const uint32_t NumCascades() const {
      return mNumCascades;
   };
private:
   bool IndexValid(uint32_t aCascadeIndex) const;
   uint32_t mNumCascades;
   std::vector<Shadow> mShadows;
};

