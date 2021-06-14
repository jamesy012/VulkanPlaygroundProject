#include "stdafx.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Buffer.h"

void Image::LoadImage(std::string aPath) {
   int width;
   int height;
   int texChannels;
   stbi_uc* pixels = stbi_load(aPath.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
   mDataSize = width * height * 4;
   mSize.width = width;
   mSize.height = height;

   if (!pixels) {
      ASSERT(false);
   }

   BufferStaging staging;
   staging.Create(mDataSize);
   {
      void* data;
      staging.Map(&data);
      memcpy(data, pixels, mDataSize);
      staging.UnMap();
   }

   stbi_image_free(pixels);

   CreateImage(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

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
         mSize.width,
         mSize.height,
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

   CreateImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Image::CreateImage(VkExtent2D aSize, VkFormat aFormat, VkImageUsageFlags aUsage, VkImageAspectFlags aAspect) {
   mSize = aSize;
   CreateImage(aFormat, aUsage);
   CreateImageView(aFormat, aAspect);
}

void Image::Destroy() {
   vkDestroyImageView(_VulkanManager->GetDevice(), mImageView, GetAllocationCallback());
   vmaDestroyImage(_VulkanManager->GetAllocator(), mImage, mAllocation);
   mImage = VK_NULL_HANDLE;
   mImageView = VK_NULL_HANDLE;
}

bool Image::CreateImage(VkFormat aFormat, VkImageUsageFlags aUsage) {
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.extent.width = mSize.width;
   imageInfo.extent.height = mSize.height;
   imageInfo.extent.depth = 1;
   imageInfo.format = aFormat;
   imageInfo.mipLevels = 1;
   imageInfo.arrayLayers = 1;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = aUsage;

   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

   VmaAllocationInfo info;
   VkResult result = vmaCreateImage(_VulkanManager->GetAllocator(), &imageInfo, &allocInfo, &mImage, &mAllocation, &info);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   return true;
}

bool Image::CreateImageView(VkFormat aFormat, VkImageAspectFlags aAspect) {
   VkImageViewCreateInfo imageViewInfo{};
   imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   imageViewInfo.format = aFormat;
   imageViewInfo.subresourceRange.aspectMask = aAspect;
   imageViewInfo.subresourceRange.baseArrayLayer = 0;
   imageViewInfo.subresourceRange.layerCount = 1;
   imageViewInfo.subresourceRange.baseMipLevel = 0;
   imageViewInfo.subresourceRange.levelCount = 1;
   imageViewInfo.image = mImage;
   VkResult result = vkCreateImageView(_VulkanManager->GetDevice(), &imageViewInfo, GetAllocationCallback(), &mImageView);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   return true;
}
