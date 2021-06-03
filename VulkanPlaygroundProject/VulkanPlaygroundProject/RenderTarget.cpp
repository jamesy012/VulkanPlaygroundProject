#include "stdafx.h"
#include "RenderTarget.h"

bool RenderTarget::Create(VkDevice aDevice, RenderPass* aRenderPass, VkExtent2D aSize, bool aIncludeDepth) {
   VkResult result;

   VkImageCreateInfo info{};
   info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
   info.format = aRenderPass->GetColorFormat();
   info.arrayLayers = 1;
   info.mipLevels = 1;
   info.initialLayout = aRenderPass->GetInitalLayoutFormat();
   info.samples = VK_SAMPLE_COUNT_1_BIT;
   info.extent.width = aSize.width;
   info.extent.height = aSize.height;
   info.extent.depth = 1;
   info.imageType = VK_IMAGE_TYPE_2D;

   //result = vkCreateImage(aDevice, &info, GetAllocationCallback(), &mColor);
   //ASSERT_VULKAN_SUCCESS_RET_FALSE(result);
   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

   VmaAllocationInfo allocationInfo;
   result = vmaCreateImage(_VulkanManager->GetAllocator(), &info, &allocInfo, &mColor, &mColorAllocation,&allocationInfo);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   VkImageViewCreateInfo viewInfo{};
   viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   viewInfo.image = mColor;
   viewInfo.format = aRenderPass->GetColorFormat();
   viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   viewInfo.subresourceRange.layerCount = 1;
   viewInfo.subresourceRange.levelCount = 1;
   result = vkCreateImageView(aDevice, &viewInfo, GetAllocationCallback(), &mColorView);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   if (aIncludeDepth) {
      info.format = aRenderPass->GetDepthFormat();
      info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      VmaAllocationInfo allocationInfo;
      result = vmaCreateImage(_VulkanManager->GetAllocator(), &info, &allocInfo, &mDepth, &mDepthAllocation, &allocationInfo);
      ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

      VkImageViewCreateInfo viewInfo{};
      viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewInfo.image = mDepth;
      viewInfo.format = info.format;
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      viewInfo.subresourceRange.layerCount = 1;
      viewInfo.subresourceRange.levelCount = 1;
      result = vkCreateImageView(aDevice, &viewInfo, GetAllocationCallback(), &mDepthView);
      ASSERT_VULKAN_SUCCESS_RET_FALSE(result);
   }

   std::vector<VkImageView> imageViews;
   imageViews.push_back(mColorView);
   imageViews.push_back(mDepthView);

   mFramebuffer.Create(aDevice, aSize, aRenderPass, imageViews);

   mExtent = aSize;

   return true;
}

void RenderTarget::Destroy() {
   const VkDevice device = _VulkanManager->GetDevice();
   mFramebuffer.Destroy(device);
   vkDestroyImageView(device, mColorView, GetAllocationCallback());
   vmaDestroyImage(_VulkanManager->GetAllocator(), mColor, mColorAllocation);
   if (mDepth != VK_NULL_HANDLE) {
      vkDestroyImageView(device, mDepthView, GetAllocationCallback());
      vmaDestroyImage(_VulkanManager->GetAllocator(), mDepth, mDepthAllocation);
      mDepth = VK_NULL_HANDLE;
      mDepthView = VK_NULL_HANDLE;
   }
}