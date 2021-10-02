#include "stdafx.h"
#include "Buffer.h"

void Buffer::CopyFrom(Buffer* aBuffer, const VkCommandBuffer aCommandList) {
   ASSERT_VULKAN_VALUE( mBuffer );
   ASSERT_VULKAN_VALUE( aBuffer );
   VkCommandBuffer commandBuffer = aCommandList;
   if (aCommandList == VK_NULL_HANDLE) {
      VulkanManager::Get()->OneTimeCommandBufferStart(commandBuffer);
   }
   VkBufferCopy copyRegion{};
   copyRegion.size = std::min(GetSize(), aBuffer->GetSize());
   vkCmdCopyBuffer(commandBuffer, aBuffer->GetBuffer(), GetBuffer(), 1, &copyRegion);
   if (aCommandList == VK_NULL_HANDLE) {
      VulkanManager::Get()->OneTimeCommandBufferEnd(commandBuffer);
   }
}

bool Buffer::Create(VkDeviceSize aSize, VkBufferUsageFlags aUseage, VmaMemoryUsage aMemUsage) {

   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = aSize;
   bufferInfo.usage = aUseage;

   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = aMemUsage;

   VmaAllocationInfo info;
   VkResult result = vmaCreateBuffer(VulkanManager::Get()->GetAllocator(), &bufferInfo, &allocInfo, &mBuffer, &mAllocation, &info);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   mSize = aSize;
   mAllocatedSize = info.size;
   return true;
}

void Buffer::Destroy() {
   ASSERT_VULKAN_VALUE( mBuffer );
   vmaDestroyBuffer(VulkanManager::Get()->GetAllocator(), mBuffer, mAllocation);
}

void Buffer::Map(void** aData) {
   ASSERT_VULKAN_VALUE( mBuffer );
   if (mMapCounter++ == 0) {
      vmaMapMemory(VulkanManager::Get()->GetAllocator(), mAllocation, &mMapPtr);
   }
   if (aData != nullptr) {
      *aData = mMapPtr;
   }
}

void Buffer::UnMap() {
   ASSERT_VULKAN_VALUE( mBuffer );
   if (--mMapCounter == 0) {
      vmaUnmapMemory(VulkanManager::Get()->GetAllocator(), mAllocation);
   }
}
