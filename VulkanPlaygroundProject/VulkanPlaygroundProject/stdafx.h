#pragma once
#pragma warning(disable: 26812)

#include <iostream>
#include <assert.h>
#include <vector>
#include <iostream>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include "VulkanManager.h"

extern class VulkanManager* _VulkanManager;

#define ASSERT_RET(x) assert(false);
#define ASSERT_RET_FALSE(x) assert(false); return false;

static void CheckVulkanResult(VkResult aResult) {
   if (aResult == VK_SUCCESS) {
      return;
   }
   fprintf(stderr, "[vulkan] Error: VkResult = %d\n", aResult);
   assert(false);
}

#define ASSERT_VULKAN_VALUE(x) assert(x != VK_NULL_HANDLE);
#define ASSERT_VULKAN_SUCCESS(x) assert(x == VK_SUCCESS);
#define ASSERT_VULKAN_SUCCESS_RET_FALSE(x) if(x != VK_SUCCESS){ASSERT_RET_FALSE("")};
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

static std::string GetWorkDir() {
   return "../WorkDir/";
}

static int RoundUp(int number, int multiple) {
   assert(multiple);
   int isPositive = (int)(number >= 0);
   return ((number + isPositive * (multiple - 1)) / multiple) * multiple;
}

static VkDeviceSize RoundUp(int number, VkDeviceSize multiple) {
   assert(multiple);
   int isPositive = (int)(number >= 0);
   return ((number + isPositive * (multiple - 1)) / multiple) * multiple;
}

//Logging
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