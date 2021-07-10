#pragma once
#pragma warning(disable: 26812)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef LoadImage

#include <iostream>
#include <assert.h>
#include <vector>
#include <iostream>
#include <functional>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>


//~~~~~~~ ASSERTS/Validation
#define ASSERT(msg) DebugBreak();
#define ASSERT_IF(x) if((x) == false){ASSERT(__FUNCTION__)};
#define ASSERT_RET(x) if(!x) {ASSERT(__FUNCTION__); return;};
#define ASSERT_RET_VALUE(x, ret) if(!(x)) {ASSERT(__FUNCTION__); return (ret);};
#define ASSERT_RET_FALSE(x) ASSERT(__FUNCTION__); return false;

static void CheckVulkanResult(VkResult aResult) {
   if (aResult == VK_SUCCESS) {
      return;
   }
   fprintf(stderr, "[vulkan] Error: VkResult = %d\n", aResult);
   assert(false);
}

#define ASSERT_VULKAN_VALUE(x) ASSERT_IF(x != VK_NULL_HANDLE);
#define ASSERT_VULKAN_SUCCESS(x) ASSERT_IF(x == VK_SUCCESS);
#define ASSERT_VULKAN_SUCCESS_RET_FALSE(x) if(x != VK_SUCCESS){ASSERT_RET_FALSE("")};
#define ASSERT_VALID(x) ASSERT_IF(x != nullptr);


//~~~~~~~ CONSTANTS

#define NUM_SHADOW_CASCADES 4
static_assert(NUM_SHADOW_CASCADES <= 4);//shader splits in vec4

//~~~~~~~ OBJECTS

struct SceneSimpleUBO {
   glm::mat4 mViewProj;//camera viewProj
};

struct SceneUBO {
   glm::mat4 mViewProj;//camera viewProj
   glm::vec4 mViewPos; //xyz, w unused
   glm::vec4 mLightPos; //xyz, w unused
   glm::vec4 mShadowSplits;//x = cascade 0, y = cascade 1 ...
   glm::mat4 mShadowCascadeProj[NUM_SHADOW_CASCADES];//shadow projView per cascade
};

struct ObjectUBO {
   glm::mat4 mModel;
};

//~~~~~~~ VULKAN HELPERS
#define SIZEOF_ARRAY(x) sizeof(x) / sizeof(x[0]);

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

static VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(VkDescriptorType aType, VkShaderStageFlags aStageFlags, uint32_t aBinding) {
   VkDescriptorSetLayoutBinding binding{};
   binding.binding = aBinding;
   binding.descriptorCount = 1;
   binding.descriptorType = aType;
   binding.stageFlags = aStageFlags;
   return binding;
}

static VkWriteDescriptorSet  CreateWriteDescriptorSet(VkDescriptorType aType, VkDescriptorSet aDstSet, VkDescriptorBufferInfo* aBufferInfo, uint32_t aBinding) {
   VkWriteDescriptorSet set{};
   set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   set.descriptorCount = 1;
   set.pBufferInfo = aBufferInfo;
   set.descriptorType = aType;

   set.dstBinding = aBinding;
   set.dstSet = aDstSet;
   return set;
}

static VkWriteDescriptorSet  CreateWriteDescriptorSet(VkDescriptorType aType, VkDescriptorSet aDstSet, VkDescriptorImageInfo* aImageInfo, uint32_t aBinding) {
   VkWriteDescriptorSet set{};
   set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   set.descriptorCount = 1;
   set.pImageInfo = aImageInfo;
   set.descriptorType = aType;

   set.dstBinding = aBinding;
   set.dstSet = aDstSet;
   return set;
}

static void SetImageLayout(VkCommandBuffer aBuffer, VkImage aImage, VkImageAspectFlags aAspectMask, VkImageLayout aOldImageLayout,
                           VkImageLayout aNewImageLayout, VkPipelineStageFlags aSrcStages, VkPipelineStageFlags aDestStages) {

   VkImageMemoryBarrier image_memory_barrier = {};
   image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   image_memory_barrier.pNext = NULL;
   image_memory_barrier.srcAccessMask = 0;
   image_memory_barrier.dstAccessMask = 0;
   image_memory_barrier.oldLayout = aOldImageLayout;
   image_memory_barrier.newLayout = aNewImageLayout;
   image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   image_memory_barrier.image = aImage;
   image_memory_barrier.subresourceRange.aspectMask = aAspectMask;
   image_memory_barrier.subresourceRange.baseMipLevel = 0;
   image_memory_barrier.subresourceRange.levelCount = 1;
   image_memory_barrier.subresourceRange.baseArrayLayer = 0;
   image_memory_barrier.subresourceRange.layerCount = 1;

   switch (aOldImageLayout) {
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
         image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
         image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_PREINITIALIZED:
         image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
         break;

      default:
         break;
   }

   switch (aNewImageLayout) {
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
         break;

      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         break;

      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
         break;

      default:
         break;
   }

   vkCmdPipelineBarrier(aBuffer, aSrcStages, aDestStages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

constexpr VkViewport GetViewportFromExtent2D(const VkExtent2D aExtent) {
   return { 0, 0, static_cast<float>(aExtent.width), static_cast<float>(aExtent.height), 0, 1 };
}

constexpr VkRect2D GetRect2DFromExtent2D(const VkExtent2D aExtent) {
   return { { 0, 0 }, aExtent };
}

//~~~~~~~~~~ PROJECT HELPERS

static std::string GetWorkDir() {
   return "../WorkDir/";
}

enum RenderMode {
   NORMAL = 1,
   SHADOW = 2,
   ALL = ~0
};

//~~~~~~~ Math Helpers

static int RoundUp(int number, int multiple) {
   assert(multiple);
   int isPositive = (int)(number >= 0);
   return ((number + isPositive * (multiple - 1)) / multiple) * multiple;
}

static VkDeviceSize RoundUp(int number, VkDeviceSize multiple) {
   return RoundUp(number, (int)multiple);
}

//~~~~~~ Logging
namespace Logger {
   extern int mPrintIndent;
   extern std::string mLogCat;

   void LogMessage(const char* aMessage, ...);

   class LogScopedIndent {
   public:
      LogScopedIndent() {
         Logger::mPrintIndent++;
      }
      ~LogScopedIndent() {
         Logger::mPrintIndent--;
      }
   };

   class LogScopedName {
   public:
      LogScopedName(std::string aName) {
         mPreviousName = Logger::mLogCat;
         Logger::mLogCat = aName;
      }
      ~LogScopedName() {
         Logger::mLogCat = mPreviousName;

      }
   private:
      std::string mPreviousName;
   };

};

#define _INTERNAL_LOG_INDENT_NAMED(x,y)			Logger::LogScopedIndent x##y;
#define LOG_SCOPED_INDENT_NAMED(x)				_INTERNAL_LOG_INDENT_NAMED(ScopedLogIndent_,x)
#define LOG_SCOPED_INDENT()						LOG_SCOPED_INDENT_NAMED( __LINE__ )

#define _INTERNAL_LOG_NAME_NAMED(var, var2, name)	Logger::LogScopedName var##var2 (name);
#define LOG_SCOPED_NAME_NAMED(var, name)			_INTERNAL_LOG_NAME_NAMED(LogName_,var, name)
#define LOG_SCOPED_NAME(name)						LOG_SCOPED_NAME_NAMED(__LINE__, name)
//#define LOG_NAME(name)							Logger::LogScopedName LogName (name);

#define LOG_GET_NAME ((const std::string)Logger::mLogCat)

#define LOG(...) Logger::LogMessage(__VA_ARGS__);
//#define LOG_LIT(Message, ...) Logger::LogMessage("%s\n", #Message, __VA_ARGS__);
#define LOG_WITH_LINE(Message) Logger::LogMessage("(%s #%i) %s", __FUNCTION__, __LINE__, Message);

//~~~~~~~ VULKAN OBJECTS
#include "VulkanManager.h"

extern class VulkanManager* _VulkanManager;

#include "Profiler.h"

template <class T>
static void DebugSetObjName(VkObjectType aType, T aObject, std::string aName) {
   _VulkanManager->DebugSetName(aType, (uint64_t)aObject, aName);
}
