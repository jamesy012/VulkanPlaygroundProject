#pragma once
class Image {
public:
    static Image* WHITE;
public:
   bool LoadImage(std::string aPath);
   bool LoadImageData(const int aWidth, const int aHeight, const unsigned char* aData);
   bool LoadImageForArray(std::string aPath, uint32_t aArrayIndex);
   bool CreateImageEmpty(const int aWidth, const int aHeight);

   //todo - remove these CreateImage?
   bool CreateImage(VkExtent2D aSize, VkFormat aFormat, VkImageUsageFlags aUsage, VkImageAspectFlags aAspect, uint32_t aNumArrays = 1u, uint32_t aNumMips = 1u);
   bool CreateImage(VkExtent2D aSize, uint32_t aNumArrays = 1u, uint32_t aNumMips = 1u);

   void SetImageData(const unsigned char* aData);
   void SetImageData(const VkExtent2D aStart, const VkExtent2D aSize, const unsigned char* aData);

   const VkImage GetImage() const {
      return mImage;
   }

   const VkImageView GetArrayImageView(uint32_t aIndex) const {
      return mArrayImageViews[aIndex];
   }

   const VkImageView GetImageView() const {
      return mImageView;
   }

   const VkImageLayout GetImageLayout() const {
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   }

   void SetName(std::string aName);

   void Destroy();

   uint32_t GetWidth() const {
       return mSize.width;
   }

   uint32_t GetHeight() const {
       return mSize.height;
   }

   uint32_t GetStride() const {
       return 4;
   }

   //temp
   bool mIncludeTransferSrcBit = false;

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
   bool mHasData = false;
};

//static void UpdateImageDescriptorSet(const VkImageLayout aImageLayout, const VkImageView aImageView, VkDescriptorSet aDescriptorSet, VkSampler aSampler, uint32_t aBinding) {
//   VkDescriptorImageInfo imgInfo;
//   VkWriteDescriptorSet writeSet = GetWriteDescriptorSet(imgInfo, aImageLayout, aImageView, aDescriptorSet, aSampler, aBinding);
//   vkUpdateDescriptorSets(VulkanManager::Get()->GetDevice(), 1, &writeSet, 0, nullptr);
//}
//
//static void UpdateImageDescriptorSet(const Image* aImage, VkDescriptorSet aDescriptorSet, VkSampler aSampler, uint32_t aBinding) {
//   UpdateImageDescriptorSet(aImage->GetImageLayout(), aImage->GetImageView(), aDescriptorSet, aSampler, aBinding);
//}

