#pragma once
class RenderPass {
public:
   bool Create(VkDevice aDevice, VkFormat aColorFormat, VkImageLayout aInital, VkImageLayout aFinal);
   void Destroy(VkDevice aDevice);

   const VkRenderPass GetRenderPass() const {
      return mRenderPass;
   }
   const VkFormat GetColorFormat() const {
      return mColorFormat;
   }
   const VkImageLayout GetInitalLayoutFormat() const {
      return mInitalLayoutFormat;
   }
private:
   VkRenderPass mRenderPass = VK_NULL_HANDLE;
   VkFormat mColorFormat;
   VkImageLayout mInitalLayoutFormat;
};

