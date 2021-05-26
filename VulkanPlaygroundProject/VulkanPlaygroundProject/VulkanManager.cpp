#include "stdafx.h"
#include "VulkanManager.h"

#include <vector>
#include <optional>
#include <set>

#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>

#include "Window.h"

const std::vector<const char*> validationLayers = {
   "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> deviceExtensions = {
   VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT messageType,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   void* pUserData) {

   std::cerr << "VULKAN: " << pCallbackData->pMessageIdName << ":\n\t" << pCallbackData->pMessage << std::endl;

   return VK_FALSE;
}

bool CheckVkLayerSupport(const std::vector<const char*> aLayersToCheck) {
   uint32_t layerCount;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

   for (const char* layerName : aLayersToCheck) {
      bool layerFound = false;
      for (const auto& layerProperties : availableLayers) {
         if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
         }
      }
      if (layerFound == false) {
         return false;
      }
   }
   return true;
}


void VulkanManager::Create(Window* aWindow) {
   CreateInstance();
   if (aWindow->CreateSurface(GetInstance())) {
      mSurface = aWindow->GetSurface();
   }
   CreateDevice();
}

void VulkanManager::Destroy(Window* aWindow) {
   vkDestroyDevice(mDevice, CreateAllocationCallbacks());

   aWindow->DestroySurface(GetInstance());

   if(enableValidationLayers && mDebugMessenger != VK_NULL_HANDLE) {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
         func(mInstance, mDebugMessenger, GetAllocationCallback());
      }
   }

   vkDestroyInstance(mInstance, GetAllocationCallback());
}

bool VulkanManager::CreateInstance() {
   if (enableValidationLayers && !CheckVkLayerSupport(validationLayers)) {
      ASSERT_RET_FALSE("validation layers requested, but not available!");
   }

   VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
   debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugCreateInfo.pfnUserCallback = DebugCallback;

   VkApplicationInfo appInfo = {};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Hello Triangle";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "No Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   //appInfo.apiVersion = VK_API_VERSION_1_0;//VK_HEADER_VERSION_COMPLETE;
   appInfo.apiVersion = VK_HEADER_VERSION_COMPLETE;

   VkInstanceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;

   std::vector<const char*> extensions;
   {
      uint32_t glfwExtensionCount = 0;
      const char** glfwExtensions;
      glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

      extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

      if (enableValidationLayers) {
         extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
   }

   createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   createInfo.ppEnabledExtensionNames = extensions.data();

   if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();

      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
   } else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
   }

   //uint32_t vkExtensionCount = 0;
   //vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
   //std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
   //vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());
   //std::cout << "available extensions:\n";
   //
   //for (const auto& extension : vkExtensions) {
   //   std::cout << '\t' << extension.extensionName << '\n';
   //}

   VkResult result = vkCreateInstance(&createInfo, GetAllocationCallback(), &mInstance);
   if (result != VK_SUCCESS) {
      ASSERT_RET_FALSE();
   }

   if (enableValidationLayers) {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
      if (func != nullptr) {
         return func(mInstance, &debugCreateInfo, GetAllocationCallback(), &mDebugMessenger);
      }
   }

   return true;
}

struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
   }
};

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice aDevice, VkSurfaceKHR aSurface) {
   QueueFamilyIndices indices;

   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueFamilyCount, nullptr);

   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueFamilyCount, queueFamilies.data());

   int i = 0;
   for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         indices.graphicsFamily = i;
      }
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(aDevice, i, aSurface, &presentSupport);
      if (presentSupport) {
         indices.presentFamily = i;
      }

      i++;
   }

   return indices;
}

struct SwapChainSupportDetails {
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice aDevice, VkSurfaceKHR aSurface) {
   SwapChainSupportDetails details;

   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aDevice, aSurface, &details.capabilities);

   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(aDevice, aSurface, &formatCount, nullptr);

   if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(aDevice, aSurface, &formatCount, details.formats.data());
   }

   uint32_t presentModeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(aDevice, aSurface, &presentModeCount, nullptr);

   if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(aDevice, aSurface, &presentModeCount, details.presentModes.data());
   }

   return details;
}

bool CheckDeviceExpensionSupport(VkPhysicalDevice aDevice) {
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, nullptr);

   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, availableExtensions.data());

   std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

   for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
   }

   return requiredExtensions.empty();
}

bool IsDeviceSuitable(VkPhysicalDevice aDevice, VkSurfaceKHR aSurface) {
   //VkPhysicalDeviceProperties deviceProperties;
   //vkGetPhysicalDeviceProperties(device, &deviceProperties);
   //VkPhysicalDeviceFeatures deviceFeatures;
   //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
   //
   //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
   //   deviceFeatures.geometryShader;

   QueueFamilyIndices indices = FindQueueFamilies(aDevice, aSurface);

   bool extensionsSupported = CheckDeviceExpensionSupport(aDevice);

   bool swapChainAdequte = false;
   if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(aDevice, aSurface);
      swapChainAdequte = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
   }

   VkPhysicalDeviceFeatures supportedFeatures;
   vkGetPhysicalDeviceFeatures(aDevice, &supportedFeatures);

   return indices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequte && supportedFeatures.samplerAnisotropy;
}

bool VulkanManager::CreateDevice() {
   ASSERT_VULKAN_VALUE(mInstance);
   //physical device
   {
      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
      if (deviceCount == 0) {
         ASSERT_RET_FALSE("failed to find GPUs with vulkan support");
      }

      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

      for (const auto& device : devices) {
         if (IsDeviceSuitable(device, mSurface)) {
            mPhysicalDevice = device;
            break;
         }
      }

      if (mPhysicalDevice == VK_NULL_HANDLE) {
         ASSERT_RET_FALSE("failed to find a suitable GPU!");
      }

      vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);
      vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mDeviceFeatures);
   }

   //logical device
   {
      QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice, mSurface);

      std::vector< VkDeviceQueueCreateInfo> queueCreateInfos;
      std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

      float queuePriority = 1.0f;
      for (uint32_t queueFamily : uniqueQueueFamilies) {
         VkDeviceQueueCreateInfo queueCreateInfo{};
         queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
         queueCreateInfo.queueFamilyIndex = queueFamily;
         queueCreateInfo.queueCount = 1;
         queueCreateInfo.pQueuePriorities = &queuePriority;

         queueCreateInfos.push_back(queueCreateInfo);
      }

      VkPhysicalDeviceFeatures deviceFeatures{};
      deviceFeatures.samplerAnisotropy = VK_TRUE;
      VkDeviceCreateInfo deviceCreateInfo{};
      deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
      deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
      deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

      if (enableValidationLayers) {
         deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
         deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
      } else {
         deviceCreateInfo.enabledLayerCount = 0;
      }

      deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
      deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

      if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, GetAllocationCallback(), &mDevice) != VK_SUCCESS) {
         ASSERT_RET_FALSE("failed to create logical device!");
      }

      vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
      vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
   }

   return true;
}
