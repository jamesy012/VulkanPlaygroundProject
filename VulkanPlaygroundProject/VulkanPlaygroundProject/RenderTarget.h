#pragma once
class RenderTarget {
public:
   bool Create(VkDevice aDevice, RenderPass* aRenderPass, VkExtent2D aSize, bool aIncludeDepth);
   void Destroy();

   void StartRenderPass(VkCommandBuffer aBuffer, std::vector<VkClearValue> aClearColors);
   void EndRenderPass(VkCommandBuffer aBuffer);

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
   VkImage mColor = VK_NULL_HANDLE;
   VmaAllocation mColorAllocation;
   VkImage mDepth = VK_NULL_HANDLE;
   VmaAllocation mDepthAllocation;
   VkImageView mColorView = VK_NULL_HANDLE;
   VkImageView mDepthView = VK_NULL_HANDLE;
   Framebuffer mFramebuffer;
   VkExtent2D mExtent;

   RenderPass* mRenderPass;
};

