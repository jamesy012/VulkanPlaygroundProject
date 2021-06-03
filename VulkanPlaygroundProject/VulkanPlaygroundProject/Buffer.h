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

   const VkDeviceSize GetAllocatedSize() const {
      return mAllocatedSize;
   }

   //copies the data from aBuffer to this buffer via commandList
   void CopyFrom(Buffer* aBuffer, const VkCommandBuffer aCommandList = VK_NULL_HANDLE);

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
   void Bind(VkCommandBuffer aBuffer) {
      VkBuffer vertexBuffers[] = { GetBuffer() };
      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(aBuffer, 0, 1, vertexBuffers, offsets);
   }
};

class BufferIndex : public Buffer {
public:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
   }
   void Bind(VkCommandBuffer aBuffer) {
      vkCmdBindIndexBuffer(aBuffer, GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
   }
};

class BufferStaging : public Buffer {
public:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
   }
};

class BufferUniform : public Buffer {
public:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
   }
};
