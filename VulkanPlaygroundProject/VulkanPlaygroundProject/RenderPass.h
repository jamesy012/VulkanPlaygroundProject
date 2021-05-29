#pragma once
class RenderPass {
public:
   bool Create(VkDevice aDevice, VkImageLayout aColorLayout, VkFormat aColorFormat);
   void Destroy(VkDevice aDevice);

   const VkRenderPass GetRenderPass() const {
      return mRenderPass;
   }
private:
   VkRenderPass mRenderPass = VK_NULL_HANDLE;
};

