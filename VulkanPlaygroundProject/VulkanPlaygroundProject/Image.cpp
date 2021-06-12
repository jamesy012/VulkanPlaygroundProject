#include "stdafx.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Buffer.h"

void Image::LoadImage(std::string aPath) {
   int texChannels;
   stbi_uc* pixels = stbi_load(aPath.c_str(), &mWidth, &mHeight, &texChannels, STBI_rgb_alpha);
   mSize = mWidth * mHeight * 4;

   if (!pixels) {
      ASSERT(false);
   }

   BufferStaging staging;
   staging.Create(mSize);
   {
      void* data;
      staging.Map(&data);
      memcpy(data, pixels, mSize);
      staging.UnMap();
   }

   stbi_image_free(pixels);

   CreateImage();

   {
      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;

      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;

      region.imageOffset = { 0, 0, 0 };
      region.imageExtent = {
         (uint32_t)mWidth,
         (uint32_t)mHeight,
         1
      };

      VkCommandBuffer commandBuffer;
      _VulkanManager->OneTimeCommandBufferStart(commandBuffer);
      SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      vkCmdCopyBufferToImage(
         commandBuffer,
         staging.GetBuffer(),
         mImage,
         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         1,
         &region
      );
      SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
      _VulkanManager->OneTimeCommandBufferEnd(commandBuffer);
   }

   staging.Destroy();

   {
      VkImageViewCreateInfo imageViewInfo{};
      imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
      imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageViewInfo.subresourceRange.baseArrayLayer = 0;
      imageViewInfo.subresourceRange.layerCount = 1;
      imageViewInfo.subresourceRange.baseMipLevel = 0;
      imageViewInfo.subresourceRange.levelCount = 1;
      imageViewInfo.image = mImage;
      vkCreateImageView(_VulkanManager->GetDevice(), &imageViewInfo, GetAllocationCallback(), &mImageView);
   }
}

void Image::Destroy() {
   vkDestroyImageView(_VulkanManager->GetDevice(), mImageView, GetAllocationCallback());
   vmaDestroyImage(_VulkanManager->GetAllocator(), mImage, mAllocation);
}

bool Image::CreateImage() {
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.extent.width = mWidth;
   imageInfo.extent.height = mHeight;
   imageInfo.extent.depth = 1;
   imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
   imageInfo.mipLevels = 1;
   imageInfo.arrayLayers = 1;
   imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;;

   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

   VmaAllocationInfo info;
   VkResult result = vmaCreateImage(_VulkanManager->GetAllocator(), &imageInfo, &allocInfo, &mImage, &mAllocation, &info);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   return true;
}
