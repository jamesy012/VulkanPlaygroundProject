#include "stdafx.h"
#include "Buffer.h"

Buffer::~Buffer() {
	ASSERT_IF(mBuffer == VK_NULL_HANDLE);
}

void Buffer::CopyFrom(Buffer* aBuffer, VkDeviceSize aOffset, VkDeviceSize aFromOffset, VkDeviceSize aSize, const VkCommandBuffer aCommandList) {
	ASSERT_VULKAN_VALUE(mBuffer);
	ASSERT_VULKAN_VALUE(aBuffer);
	if(aBuffer == VK_NULL_HANDLE) {
		return;
	}
	const VkDeviceSize copySize = aSize == -1 ? std::min(GetSize(), aBuffer->GetSize()) : aSize;//size of aBuffer if not supplied
	ASSERT_IF(aOffset + copySize <= GetSize());

	OneTimeCommandBuffer(aCommandList,
						 [&](VkCommandBuffer commandBuffer) {
							 VkBufferCopy copyRegion{};
							 //copyRegion.size = std::min( GetSize(), aBuffer->GetSize() );
							 copyRegion.size = copySize;
							 copyRegion.dstOffset = aOffset;
							 copyRegion.srcOffset = aFromOffset;
							 vkCmdCopyBuffer(commandBuffer, aBuffer->GetBuffer(), GetBuffer(), 1, &copyRegion);
						 }
	);

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
	ASSERT_VULKAN_VALUE(mBuffer);
	if(mBuffer) {
		vmaDestroyBuffer(VulkanManager::Get()->GetAllocator(), mBuffer, mAllocation);
	}
	mBuffer = VK_NULL_HANDLE;
	mAllocation = VK_NULL_HANDLE;
}

void Buffer::Map(void** aData) {
	ASSERT_VULKAN_VALUE(mBuffer);
	if(mMapCounter++ == 0) {
		vmaMapMemory(VulkanManager::Get()->GetAllocator(), mAllocation, &mMapPtr);
	}
	if(aData != nullptr) {
		*aData = mMapPtr;
	}
}

void Buffer::UnMap() {
	ASSERT_VULKAN_VALUE(mBuffer);
	if(--mMapCounter == 0) {
		vmaUnmapMemory(VulkanManager::Get()->GetAllocator(), mAllocation);
	}
}
