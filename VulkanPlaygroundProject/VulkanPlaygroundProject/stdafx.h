#pragma once

#include <iostream>
#include <assert.h>
#include <vector>

#include <vulkan/vulkan.h>

#define ASSERT_RET(x) assert(false);
#define ASSERT_RET_FALSE(x) assert(false); return false;

#define ASSERT_VULKAN_VALUE(x) assert(x != VK_NULL_HANDLE);
#define ASSERT_VULKAN_SUCCESS(x) assert(x == VK_SUCCESS);
#define ASSERT_VALID(x) assert(x != nullptr);

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