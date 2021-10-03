#pragma once

class Buffer {
public:
   ~Buffer();

   virtual bool Create(VkDeviceSize aSize) = 0;
   void Destroy();
   
   void Map(void** aData);
   void UnMap();

   void SetName(std::string aName) {
      ASSERT_VULKAN_VALUE( mBuffer );
      DebugSetObjName(VK_OBJECT_TYPE_BUFFER, mBuffer, aName);
   }

   const VkBuffer GetBuffer() const {
      ASSERT_VULKAN_VALUE( mBuffer );
      return mBuffer;
   }
   const VkDeviceSize GetSize() const {
      ASSERT_IF( mSize != 0);
      return mSize;
   }

   const VkDeviceSize GetAllocatedSize() const {
      ASSERT_IF( mAllocatedSize != 0 );
      return mAllocatedSize;
   }

   //copies the data from aBuffer to this buffer via commandList
   void CopyFrom(Buffer* aBuffer, const VkCommandBuffer aCommandList = VK_NULL_HANDLE);

protected:
   bool Create(VkDeviceSize aSize, VkBufferUsageFlags aUseage, VmaMemoryUsage aMemUsage);
private:
   VkBuffer mBuffer = VK_NULL_HANDLE;
   VmaAllocation mAllocation = VK_NULL_HANDLE;
   VkDeviceSize mSize = 0;
   VkDeviceSize mAllocatedSize = 0;
   //number of times this is mapped, to track when to unmap or map the data
   uint32_t mMapCounter = 0;
   void* mMapPtr = nullptr;
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
   bool Create(uint32_t aCount, VkDeviceSize aStructSize) {
      mStructSize = aStructSize;
      mAllignedStructSize = RoundUp((int)aStructSize, VulkanManager::Get()->GetUniformBufferAllignment());
      mCount = aCount;
      bool res = Create(aCount * (VkDeviceSize)mAllignedStructSize);
      if (!res) {
         return false;
      }
      return true;
   }
   bool Create(uint32_t aCount, VkDeviceSize aStructSize, VkDescriptorSet aDescriptorSet, uint32_t aBinding, bool aDynamic = true) {
      if (!Create(aCount, aStructSize)) {
         return false;
      }

      AddToDescriptorSet(aDescriptorSet, aBinding, aDynamic);
      return true;
   }

   void Destroy() {
      //vkFreeDescriptorSets(VulkanManager::Get()->GetDevice(), mDescriptorPool, 1, &mDescriptorSet); //needs VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT and needs to know the pool too
      Buffer::Destroy();
   }

   void AddToDescriptorSet(VkDescriptorSet aDescriptorSet, uint32_t aBinding, bool aDynamic = true) {
      if ( aDescriptorSet == VK_NULL_HANDLE )
      {
         return;
      }
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = GetBuffer();
      bufferInfo.range = mStructSize;
      VkWriteDescriptorSet sceneSet = CreateWriteDescriptorSet(GetDescriptorType(aDynamic), aDescriptorSet, &bufferInfo, aBinding);
      vkUpdateDescriptorSets(VulkanManager::Get()->GetDevice(), 1, &sceneSet, 0, nullptr);
   }

   void SetName(std::string aName) {
      Buffer::SetName(aName);
   }

   const VkDeviceSize GetStructSize() const {
      return mStructSize;
   } 
   
   const VkDeviceSize GetSize() const {
      return Buffer::GetSize();
   } 
   
   const uint32_t GetCount() const {
      return mCount;
   }

   void Map(void** aData) {
      Buffer::Map(aData);
   };

   void UnMap() {
      Buffer::UnMap();
   }

protected:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
   }
   virtual VkDescriptorType GetDescriptorType(bool aDynamic) {
      if (aDynamic) {
         return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      }
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   }
   uint32_t mCount = 0;
   VkDeviceSize mStructSize;
   VkDeviceSize mAllignedStructSize;
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
         uint32_t frame = VulkanManager::Get()->GetCurrentFrameCounter();
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
   uint32_t mLastOverflowFrame = -1;
#endif
};

class BufferStorageUniform : public BufferUniform {
public:
   bool Create(uint32_t aCount, VkDeviceSize aStructSize, VkDescriptorSet aDescriptorSet, uint32_t aBinding, bool aDynamic = true) {
      return BufferUniform::Create(aCount, aStructSize, aDescriptorSet, aBinding, aDynamic);
   }

   const VkBuffer GetBuffer() const
   {
      return Buffer::GetBuffer();
   }

private:
   bool Create(VkDeviceSize aSize) override {
      return Buffer::Create(aSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
   }
   VkDescriptorType GetDescriptorType(bool aDynamic) override {
      if (aDynamic) {
         return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
      }
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
   }
};


struct DescriptorUBO {
public:
   DescriptorUBO(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, BufferRingUniform* aObjectBuffer, VkDescriptorSet aDescriptorSet) {
      mCommandBuffer = aCommandBuffer;
      mPipelineLayout = aPipelineLayout;
      mObjectBuffer = aObjectBuffer;

      mDescriptorSet = aDescriptorSet;

      mObjectBuffer->Get();
   }

   ~DescriptorUBO() {
      mObjectBuffer->Return();
   }

   void UpdateObjectAndBind(void* aData) {
      void* objectUbo;
      mObjectBuffer->Get((void**)&objectUbo);
      memcpy(objectUbo, aData, mObjectBuffer->GetStructSize());
      mObjectBuffer->Return();
      BindDescriptorSet();
   }

   void BindDescriptorSet() {
      uint32_t descriptorSetOffsets[] = { mObjectBuffer->GetCurrentOffset() };
      vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1, 1, &mDescriptorSet, 1, descriptorSetOffsets);
   }

   BufferRingUniform* mObjectBuffer;
   VkCommandBuffer mCommandBuffer;
   VkPipelineLayout mPipelineLayout;
private:
   VkDescriptorSet mDescriptorSet;
};
