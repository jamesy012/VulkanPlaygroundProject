#pragma once

class RenderPass;
class Image;
class Framebuffer;

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
   void Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout, uint32_t aNumCascades = 1);
   void Destroy();

   void StartRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex = 0);
   void EndRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex = 0);

   const class Image* GetImage() const {
      return mDepthImage;
   }

   void SetName(std::string aName);

   const uint32_t NumCascades() const {
      return mNumCascades;
   };

protected:
   bool IndexValid(uint32_t aCascadeIndex) const;

   VkExtent2D mSize;
   VkDescriptorSet mShadowSet = VK_NULL_HANDLE;

   Image* mDepthImage;
   std::vector<Framebuffer*> mFramebuffers;
   uint32_t mNumCascades;

};

class ShadowDirectional : public Shadow {
};

