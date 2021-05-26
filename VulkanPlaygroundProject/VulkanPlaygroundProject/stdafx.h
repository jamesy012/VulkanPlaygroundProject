#pragma once

#include <iostream>
#include <assert.h>

#include <vulkan/vulkan.h>

#define ASSERT_RET_FALSE() assert(false); return false;

#define ASSERT_VULKAN_VALUE(x) assert(x != VK_NULL_HANDLE);

static VkAllocationCallbacks* CreateAllocationCallbacks() {
   VkAllocationCallbacks callback;
   callback.pUserData = nullptr;
   callback.pfnAllocation = nullptr;
   callback.pfnReallocation = nullptr;
   callback.pfnFree = nullptr;
   callback.pfnInternalAllocation = nullptr;
   callback.pfnInternalFree = nullptr;
   return nullptr;
}

static VkAllocationCallbacks* GetAllocationCallback() {
   static VkAllocationCallbacks* allocationCallback = CreateAllocationCallbacks();
   return allocationCallback;
}