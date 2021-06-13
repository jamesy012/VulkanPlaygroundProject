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
   VkBuffer mBuffer = VK_NULL_HANDLE;
   VmaAllocation mAllocation = nullptr;
   VkDeviceSize mSize = 0;
   VkDeviceSize mAllocatedSize = 0;
   //number of times this is mapped, to track when to unmap or map the data
   uint32_t mMapCounter = 0;
   void* mMapPtr = nullptr;
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

class BufferUniform : protected Buffer {
public:
   bool Create(uint32_t aCount, VkDeviceSize aStructSize, VkDescriptorSet aDescriptorSet, uint32_t aBinding) {
      mStructSize = aStructSize;
      mAllignedStructSize = RoundUp((int)aStructSize, _VulkanManager->GetUniformBufferAllignment());
      mCount = aCount;
      bool res = Create(aCount * (uint32_t)mAllignedStructSize);
      if (!res) {
         return false;
      }


      mDescriptorSet = aDescriptorSet;
      mBinding = aBinding;

      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = GetBuffer();
      bufferInfo.range = mStructSize;
      VkWriteDescriptorSet sceneSet = CreateWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, aDescriptorSet, &bufferInfo, aBinding);
      vkUpdateDescriptorSets(_VulkanManager->GetDevice(), 1, &sceneSet, 0, nullptr);
      return true;
   }

   void Destroy() {
      //vkFreeDescriptorSets(_VulkanManager->GetDevice(), mDescriptorPool, 1, &mDescriptorSet); //needs VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT and needs to know the pool too
      Buffer::Destroy();
   }

   const VkDescriptorSet GetDescriptorSet() const {
      return mDescriptorSet;
   }

   const VkDeviceSize GetStructSize() const {
      return mStructSize;
   }

protected:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
   }
   uint32_t mCount;
   VkDeviceSize mStructSize;
   VkDeviceSize mAllignedStructSize;

   VkDescriptorSet mDescriptorSet;
   uint32_t mBinding;
};

class BufferRingUniform : public BufferUniform {
public:

   const uint32_t GetCurrentOffset() const {
      return mCurrentOffset;
   }

   void Get() {
      Map(nullptr);
   }

   void Get(void** aData) {
      uint32_t offset;
      Get(aData, offset);
   }

   void Get(void** aData, uint32_t& aOffset) {
      void* data;
      Map(&data);
      mCurrentOffset = aOffset = (uint32_t)(mAllignedStructSize * mOffsetCount++);
      if (aData != nullptr) {
         *aData = ((char*)data + aOffset);
      }
      if (mOffsetCount >= mCount) {
         mOffsetCount = 0;
#if defined(_DEBUG)
         uint32_t frame = _VulkanManager->GetCurrentFrameCounter();
         if (frame == mLastOverflowFrame) {
            LOG("RingBuffer being used too much per frame\n");
         }
         mLastOverflowFrame = frame;
#endif
      }
   }

   void Return() {
      UnMap();
   }
private:
   uint32_t mCurrentOffset;
   uint16_t mOffsetCount = 0;
#if defined(_DEBUG)
   uint32_t mLastOverflowFrame = 0;
#endif
};
