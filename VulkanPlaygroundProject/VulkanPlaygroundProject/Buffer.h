#pragma once
class Buffer {
public:
   virtual bool Create(VkDeviceSize aSize) = 0;
   void Destroy();
   
   void Map(void** aData);
   void UnMap();

   const VkBuffer GetBuffer() const {
      return mBuffer;
   }
   const VkDeviceSize GetSize() const {
      return mSize;
   }
protected:
   bool Create(VkDeviceSize aSize, VkBufferUsageFlags aUseage, VmaMemoryUsage aMemUsage);
private:
   VkBuffer mBuffer;
   VmaAllocation mAllocation;
   VkDeviceSize mSize = 0;
   VkDeviceSize mAllocatedSize = 0;
};

class BufferVertex : public Buffer { 
public:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
   }
};

class BufferStaging : public Buffer {
public:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
   }
};


