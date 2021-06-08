#pragma once
class Image {
public:
   void LoadImage(std::string aPath);

   const VkImage GetImage() const {
      return mImage;
   }

   const VkImageView GetImageView() const {
      return mImageView;
   }

   const VkImageLayout GetImageLayout()const {
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   }

   void Destroy();
private:
   bool CreateImage();

   VkImage mImage = VK_NULL_HANDLE;
   VkImageView mImageView = VK_NULL_HANDLE;
   VmaAllocation mAllocation;

   int mWidth;
   int mHeight;
   int mSize;
};

class ImageDes {
public:
   ImageDes(Image* aImage, VkDescriptorSet aDescriptorSet, VkSampler aSampler, uint32_t aBinding) {
      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageView = aImage->GetImageView();
      imageInfo.imageLayout = aImage->GetImageLayout();
      imageInfo.sampler = aSampler;
      VkWriteDescriptorSet sceneSet = CreateWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aDescriptorSet, &imageInfo, aBinding);
      vkUpdateDescriptorSets(_VulkanManager->GetDevice(), 1, &sceneSet, 0, nullptr);
   }
};
