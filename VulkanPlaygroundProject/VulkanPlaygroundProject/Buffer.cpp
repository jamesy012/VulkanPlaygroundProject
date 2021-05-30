#include "stdafx.h"
#include "Buffer.h"

bool Buffer::Create(VkDeviceSize aSize, VkBufferUsageFlags aUseage, VmaMemoryUsage aMemUsage) {

   VkBufferCreateInfo bufferInfo{};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = aSize;
   bufferInfo.usage = aUseage;

   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = aMemUsage;

   VmaAllocationInfo info;
   VkResult result = vmaCreateBuffer(_VulkanManager->GetAllocator(), &bufferInfo, &allocInfo, &mBuffer, &mAllocation, &info);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   mSize = aSize;
   mAllocatedSize = info.size;
   return true;
}

void Buffer::Destroy() {
   vmaDestroyBuffer(_VulkanManager->GetAllocator(), mBuffer, mAllocation);
}

void Buffer::Map(void** aData) {
   vmaMapMemory(_VulkanManager->GetAllocator(), mAllocation, aData);
}

void Buffer::UnMap() {
   vmaUnmapMemory(_VulkanManager->GetAllocator(), mAllocation);
}
