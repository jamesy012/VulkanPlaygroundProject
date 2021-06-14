#pragma once
class Shadow {
public:
   void Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout);
   void Destroy();

   void StartRenderPass(VkCommandBuffer aBuffer);
   void EndRenderPass(VkCommandBuffer aBuffer);

   const class RenderPass* GetRenderPass() const {
      return mRenderPass;
   }

   const class Image* GetImage() const {
      return mDepthImage;
   }

   void SetName(std::string aName);

   //Transform mTransform;
private:
   VkExtent2D mSize;
   VkDescriptorSet mShadowSet = VK_NULL_HANDLE;

   class Image* mDepthImage;
   class RenderPass* mRenderPass;
   class Framebuffer* mFramebuffer;
};

class ShadowDirectional : public Shadow {

};

