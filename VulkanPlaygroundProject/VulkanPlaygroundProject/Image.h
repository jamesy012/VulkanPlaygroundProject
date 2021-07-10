#pragma once
class Image {
public:
   void LoadImage(std::string aPath);
   void CreateImage(VkExtent2D aSize, VkFormat aFormat, VkImageUsageFlags aUsage, VkImageAspectFlags aAspect, uint32_t aNumArrays = 1u, uint32_t aNumMips = 1u);

   const VkImage GetImage() const {
      return mImage;
   }

   const VkImageView GetArrayImageView(uint32_t aIndex) const {
      return mArrayImageViews[aIndex];
   }

   const VkImageView GetImageView() const {
      return mImageView;
   }

   const VkImageLayout GetImageLayout()const {
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   }

   void SetName(std::string aName);

   void Destroy();
private:
   bool CreateImage(VkFormat aFormat, VkImageUsageFlags aUsage);
   bool CreateImageViews(VkFormat aFormat, VkImageAspectFlags aAspect);

   VkImage mImage = VK_NULL_HANDLE;
   VkImageView mImageView = VK_NULL_HANDLE;
   std::vector<VkImageView> mArrayImageViews;
   VmaAllocation mAllocation;

   VkExtent2D mSize = { 0, 0 };
   uint32_t mNumArrays = 1u;
   uint32_t mNumMips = 1u;
   uint32_t mDataSize = 0u;
};

static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorImageInfo& aImageInfo, const Image* aImage, VkDescriptorSet aDescriptorSet, VkSampler aSampler, uint32_t aBinding) {
   aImageInfo = {};
   aImageInfo.imageView = aImage->GetImageView();
   aImageInfo.imageLayout = aImage->GetImageLayout();
   aImageInfo.sampler = aSampler;
   return CreateWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aDescriptorSet, &aImageInfo, aBinding);
}

static void UpdateImageDescriptorSet(const Image* aImage, VkDescriptorSet aDescriptorSet, VkSampler aSampler, uint32_t aBinding) {
   VkDescriptorImageInfo imgInfo;
   VkWriteDescriptorSet writeSet = GetWriteDescriptorSet(imgInfo, aImage, aDescriptorSet, aSampler, aBinding);
   vkUpdateDescriptorSets(_VulkanManager->GetDevice(), 1, &writeSet, 0, nullptr);
}
