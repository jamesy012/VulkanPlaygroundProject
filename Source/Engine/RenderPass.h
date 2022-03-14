#pragma once
class RenderPass {
public:
   bool Create(VkDevice aDevice, VkFormat aColorFormat, VkImageLayout aInital, VkImageLayout aFina, VkFormat aDepthFormat = VK_FORMAT_UNDEFINED);
   void Destroy(VkDevice aDevice);

   const VkRenderPass GetRenderPass() const {
      return mRenderPass;
   }
   const VkFormat GetColorFormat() const {
      return mColorFormat;
   }
   const VkFormat GetDepthFormat() const {
      return mDepthFormat;
   }
   const VkImageLayout GetInitalLayoutFormat() const {
      return mInitalLayoutFormat;
   }
   const bool IsValid() const {
      return mRenderPass != VK_NULL_HANDLE;
   }

   void SetName(std::string aName);

private:
   VkRenderPass mRenderPass = VK_NULL_HANDLE;
   VkFormat mColorFormat;
   VkFormat mDepthFormat;
   VkImageLayout mInitalLayoutFormat = VK_IMAGE_LAYOUT_UNDEFINED;
};

