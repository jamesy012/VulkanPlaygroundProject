#pragma once
class Image {
public:
   void LoadImage(std::string aPath);

   void Destroy();
private:
   bool CreateImage();

   VkImage mImage;
   VmaAllocation mAllocation;

   int mWidth;
   int mHeight;
   int mSize;
};

