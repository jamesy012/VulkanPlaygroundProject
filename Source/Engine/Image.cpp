#include "stdafx.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Buffer.h"

#define BITS_PER_PIXEL 4

Image* Image::WHITE;

bool Image::LoadImage(std::string aPath) {
   int width;
   int height;
   int texChannels;

   //stride hardcoded to 4
   stbi_uc* pixels = stbi_load(aPath.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

   mNumArrays = 1;
   mNumMips = 1;

   if (!pixels) {
      ASSERT(false);
      return false;
   }

   bool result = LoadImageData(width, height, pixels);

   stbi_image_free(pixels);

   SetName(aPath);
   return result;
}

bool Image::LoadImageData(const int aWidth, const int aHeight, const unsigned char* aData) {
    bool result = CreateImageEmpty(aWidth, aHeight);
    if (!result) {
        return false;
    }
    mHasData = true;

    SetImageData(aData);
    return result;
}


bool Image::CreateImageEmpty(const int aWidth, const int aHeight) {
    mSize.width = aWidth;
    mSize.height = aHeight;

    int flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mIncludeTransferSrcBit) {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    bool result = CreateImage(VK_FORMAT_R8G8B8A8_SRGB, flags);
    ASSERT_RET_VALUE(result, false);

    result = CreateImageViews(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    ASSERT_RET_VALUE(result, false);

    OneTimeCommandBuffer(VK_NULL_HANDLE, [&](VkCommandBuffer commandBuffer) {
        SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        });

    return result;
}

void Image::SetImageData(const unsigned char* aData) {
    //chould check size of aData 
    //to make sure we arent copying too far over the array?

    BufferStaging staging;
    staging.Create(mDataSize);
    {
        void* data;
        staging.Map(&data);
        memcpy(data, aData, mDataSize);
        staging.UnMap();
    }

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

        OneTimeCommandBuffer(VK_NULL_HANDLE, [&](VkCommandBuffer commandBuffer) {
            SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkCmdCopyBufferToImage(
                commandBuffer,
                staging.GetBuffer(),
                mImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
            SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            });
    }

    staging.Destroy();
}

void Image::SetImageData(const VkExtent2D aStart, const VkExtent2D aSize, const unsigned char* aData) {
    //chould check size's of data?
    //to make sure we arent copying too far over the array?


    Image img;
    img.mIncludeTransferSrcBit = true;
    img.LoadImageData(GetWidth(), aSize.height, aData + (0 + (aStart.height * GetWidth())) * img.GetStride());

    {
        VkImageBlit blit{};
        //[0] being 0,0 to get the full image
        blit.dstOffsets[0].x = 0;// aStart.width;
        blit.dstOffsets[0].y = aStart.height;
        blit.dstOffsets[0].z = 0;

        blit.srcOffsets[1].x = GetWidth();// aSize.width;
        blit.srcOffsets[1].y = aSize.height;
        blit.srcOffsets[1].z = 1;
        blit.dstOffsets[1].x = GetWidth();//aStart.width + aSize.width;
        blit.dstOffsets[1].y = aStart.height + aSize.height;
        blit.dstOffsets[1].z = 1;
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.layerCount = 1;
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.layerCount = 1;

        OneTimeCommandBuffer(VK_NULL_HANDLE, [&](VkCommandBuffer commandBuffer) {
            //no need to change this back as we delete it afterwards
            SetImageLayout(commandBuffer, img.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkCmdBlitImage(commandBuffer, img.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);

            SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            });
    }
    img.Destroy();
}


bool Image::LoadImageForArray(std::string aPath, uint32_t aArrayIndex) {
   ASSERT_IF(aArrayIndex < mNumArrays);
   int width;
   int height;
   int texChannels;
   stbi_uc* pixels = stbi_load(aPath.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

   //todo support loading images of different sizes
   if (width != mSize.width) {
      ASSERT(false);
      return false;
   }
   if (height != mSize.height) {
      ASSERT(false);
      return false;
   }

   if (!pixels) {
      ASSERT(false);
      return false;
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

   {
      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;

      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = aArrayIndex;
      region.imageSubresource.layerCount = 1;

      region.imageOffset = { 0, 0, 0 };
      region.imageExtent = {
         mSize.width,
         mSize.height,
         1
      };

      VkCommandBuffer commandBuffer;
      VulkanManager::Get()->OneTimeCommandBufferStart(commandBuffer);
      SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, aArrayIndex);
      vkCmdCopyBufferToImage(
         commandBuffer,
         staging.GetBuffer(),
         mImage,
         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         1,
         &region
      );
      SetImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, aArrayIndex);
      VulkanManager::Get()->OneTimeCommandBufferEnd(commandBuffer);
      VulkanManager::Get()->WaitDevice();
   }

   staging.Destroy();

   mHasData = true;

   return true;
}

bool Image::CreateImage(VkExtent2D aSize, VkFormat aFormat, VkImageUsageFlags aUsage, VkImageAspectFlags aAspect, uint32_t aNumArrays, uint32_t aNumMips) {
   mSize = aSize;
   mNumArrays = aNumArrays;
   mNumMips = aNumMips;
   bool result = CreateImage(aFormat, aUsage);
   ASSERT_RET_VALUE(result, false);
   result = CreateImageViews(aFormat, aAspect);
   ASSERT_RET_VALUE(result, false);
   SetName("Image");
   return true;
}

bool Image::CreateImage(VkExtent2D aSize, uint32_t aNumArrays, uint32_t aNumMips) {
   return CreateImage(aSize, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, aNumArrays, aNumMips);
}

void Image::SetName(std::string aName) {
   DebugSetObjName(VK_OBJECT_TYPE_IMAGE, GetImage(), aName + " Image");
   for (uint32_t i = 0; i < mNumArrays; i++) {
      DebugSetObjName(VK_OBJECT_TYPE_IMAGE_VIEW, GetArrayImageView(i), aName + " Image View " + std::to_string(i));
   }
}

void Image::Destroy() {
   for (uint32_t i = 0; i < mNumArrays && mNumArrays != 1; i++) {
      vkDestroyImageView(VulkanManager::Get()->GetDevice(), mArrayImageViews[i], GetAllocationCallback());
   }
   mArrayImageViews.clear();
   vkDestroyImageView(VulkanManager::Get()->GetDevice(), mImageView, GetAllocationCallback());

   vmaDestroyImage(VulkanManager::Get()->GetAllocator(), mImage, mAllocation);
   mImage = VK_NULL_HANDLE;
}

bool Image::CreateImage(VkFormat aFormat, VkImageUsageFlags aUsage) {
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.extent.width = mSize.width;
   imageInfo.extent.height = mSize.height;
   imageInfo.extent.depth = 1;
   imageInfo.format = aFormat;
   imageInfo.mipLevels = mNumMips;
   imageInfo.arrayLayers = mNumArrays;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = aUsage;

   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

   VmaAllocationInfo info;
   VkResult result = vmaCreateImage(VulkanManager::Get()->GetAllocator(), &imageInfo, &allocInfo, &mImage, &mAllocation, &info);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   mDataSize = mSize.width * mSize.height * BITS_PER_PIXEL;

   return true;
}

bool Image::CreateImageViews(VkFormat aFormat, VkImageAspectFlags aAspect) {
   VkImageViewCreateInfo imageViewInfo{};
   imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   if (mNumArrays != 1) {
      imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
   } else {
      imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   }
   imageViewInfo.format = aFormat;
   imageViewInfo.subresourceRange.aspectMask = aAspect;
   imageViewInfo.subresourceRange.baseArrayLayer = 0;
   imageViewInfo.subresourceRange.layerCount = mNumArrays;
   imageViewInfo.subresourceRange.baseMipLevel = 0;
   imageViewInfo.subresourceRange.levelCount = mNumMips;
   imageViewInfo.image = mImage;

   VkResult result = vkCreateImageView(VulkanManager::Get()->GetDevice(), &imageViewInfo, GetAllocationCallback(), &mImageView);

   mArrayImageViews.resize(mNumArrays);
   if (mNumArrays != 1) {
      imageViewInfo.subresourceRange.layerCount = 1;
      for (uint32_t i = 0; i < mNumArrays; i++) {
         imageViewInfo.subresourceRange.baseArrayLayer = i;
         VkResult result = vkCreateImageView(VulkanManager::Get()->GetDevice(), &imageViewInfo, GetAllocationCallback(), &mArrayImageViews[i]);
         ASSERT_VULKAN_SUCCESS_RET_FALSE(result);
      }
   } else {
      mArrayImageViews[0] = mImageView;
   }

   return true;
}
