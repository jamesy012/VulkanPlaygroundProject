#pragma once
class RenderTarget {
public:
   bool Create(VkDevice aDevice, RenderPass* aRenderPass, VkExtent2D aSize, bool aIncludeDepth);
   void Destroy();

   const Framebuffer GetFramebuffer() const {
      return mFramebuffer;
   };

   const VkImage GetColorImage() const {
      return mColor;
   };

   const VkExtent2D GetSize() const {
      return mExtent;
   };

   const VkViewport GetViewport() const {
      return { 0, 0, static_cast<float>(mExtent.width), static_cast<float>(mExtent.height), 0, 1 };
   }

private:
   VkImage mColor;
   VmaAllocation mColorAllocation;
   VkImage mDepth;
   VkImageView mColorView;
   VkImageView mDepthView;
   Framebuffer mFramebuffer;
   VkExtent2D mExtent;
};

