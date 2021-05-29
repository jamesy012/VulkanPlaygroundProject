#include "stdafx.h"
#include "VulkanManager.h"

#include <optional>
#include <set>

#include <vulkan/vulkan.h>

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

void VulkanManager::Create(Window* aWindow) {
   CreateInstance();
   if (aWindow->CreateSurface(GetInstance())) {
      mSurface = aWindow->GetSurface();
   }
   CreateDevice();

   CreateSwapchain(aWindow);

   CreateCommandPoolBuffers();

   CreateSyncObjects();

   {
      mPresentRenderPass.Create(mDevice, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, mSwapChainImageFormat);
      mPresentFramebuffer.resize(mNumSwapChainImages);
      for (uint32_t i = 0; i < mNumSwapChainImages; i++) {
         mPresentFramebuffer[i].Create(mDevice,mSwapChainExtent, &mPresentRenderPass, mSwapChainImageViews[i]);
      }
   }

   //{
   //   VkCommandBufferAllocateInfo allocInfo{};
   //   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   //   allocInfo.commandPool = mGraphicsCommandPool;
   //   allocInfo.commandBufferCount = 1;
   //   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   //   VkCommandBuffer buffer;
   //   vkAllocateCommandBuffers(mDevice, &allocInfo, &buffer);
   //   VkCommandBufferBeginInfo beginInfo{};
   //   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   //   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   //   vkBeginCommandBuffer(buffer, &beginInfo);
   //
   //   for (int i = 0; i < mNumSwapChainImages; i++) {
   //      VkImageMemoryBarrier barrier{};
   //      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   //      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   //      barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   //      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   //      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   //      barrier.image = mSwapChainImages[i];
   //      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   //      barrier.subresourceRange.baseMipLevel = 0;
   //      barrier.subresourceRange.levelCount = 1;
   //      barrier.subresourceRange.baseArrayLayer = 0;
   //      barrier.subresourceRange.layerCount = 1;
   //      //barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
   //
   //      vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
   //   }
   //   vkEndCommandBuffer(buffer);
   //   VkSubmitInfo submitInfo{};
   //   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   //   submitInfo.commandBufferCount = 1;
   //   submitInfo.pCommandBuffers = &buffer;
   //
   //   vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
   //   vkQueueWaitIdle(mGraphicsQueue);
   //
   //   vkFreeCommandBuffers(mDevice, mGraphicsCommandPool, 1, &buffer);
   //}
}

void VulkanManager::Destroy(Window* aWindow) {
   vkDeviceWaitIdle(mDevice);

   for (size_t i = 0; i < mNumSwapChainImages; i++) {
      vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
      vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
   }

   vkDestroySwapchainKHR(mDevice, mSwapChain, GetAllocationCallback());
   mSwapChain = VK_NULL_HANDLE;
   mSwapChainImages.clear();
   mNumSwapChainImages = 0;
   mSwapChainExtent = {};
   mSwapChainImageFormat = VK_FORMAT_UNDEFINED;

   vkFreeCommandBuffers(mDevice, mGraphicsCommandPool, static_cast<uint32_t>(mCommandBuffers.size()), mCommandBuffers.data());
   mCommandBuffers.clear();
   vkDestroyCommandPool(mDevice, mGraphicsCommandPool, GetAllocationCallback());
   mGraphicsCommandPool = VK_NULL_HANDLE;

   vkDestroyDevice(mDevice, CreateAllocationCallbacks());
   mDevice = VK_NULL_HANDLE;

   aWindow->DestroySurface(GetInstance());

   if(enableValidationLayers && mDebugMessenger != VK_NULL_HANDLE) {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
         func(mInstance, mDebugMessenger, GetAllocationCallback());
      }
   }

   vkDestroyInstance(mInstance, GetAllocationCallback());
}

bool VulkanManager::RenderStart(VkCommandBuffer& aBuffer, uint32_t& aFrameIndex) {
   assert(mCurrentImageIndex == -1);
   VkResult result = vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrameIndex], VK_NULL_HANDLE, &aFrameIndex);

   if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      //RecreateSwapChain();
      return false;
   } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      assert("failed to acqure swap chain image");
   }

   // Check if a previous frame is using this image (i.e. there is its fence to wait on)
   if (mImagesInFlight[aFrameIndex] != VK_NULL_HANDLE) {
      vkWaitForFences(mDevice, 1, &mImagesInFlight[aFrameIndex], VK_TRUE, UINT64_MAX);
   }
   // Mark the image as now being in use by this frame
   mImagesInFlight[aFrameIndex] = mInFlightFences[mCurrentFrameIndex];
   mCurrentImageIndex = aFrameIndex;
   aBuffer = mCommandBuffers[aFrameIndex];
   return true;
}

void VulkanManager::RenderSubmit(std::vector<VkCommandBuffer> aCommandBuffers) {
   assert(mCurrentImageIndex != -1);
   VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrameIndex] };

   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrameIndex] };
   VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;

   //std::array<VkCommandBuffer, 2> commandBuffersList = { mCommandBuffers[mCurrentImageIndex], imguiCommandBuffers[mCurrentImageIndex] };
   std::vector<VkCommandBuffer> commandBuffersList = { mCommandBuffers[mCurrentImageIndex] };
   submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersList.size());
   submitInfo.pCommandBuffers = commandBuffersList.data();


   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = signalSemaphores;

   //vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
   vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrameIndex]);

   if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrameIndex]) != VK_SUCCESS) {
      assert("failed to submit draw command buffer!");
   }
}

void VulkanManager::RenderEnd() {
   assert(mCurrentImageIndex != -1);
   RenderSubmit({ mCommandBuffers[mCurrentImageIndex] });

   VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrameIndex] };
   VkPresentInfoKHR presentInfo{};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores = signalSemaphores;
   VkSwapchainKHR swapChains[] = { mSwapChain };
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = swapChains;
   uint32_t imageIndices = static_cast<uint32_t>(mCurrentImageIndex);
   presentInfo.pImageIndices = &imageIndices;
   presentInfo.pResults = nullptr; // Optional

   {
      vkQueueWaitIdle(mPresentQueue);
   }

   VkResult result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR /*|| framebufferResized*/) {
      //RecreateSwapChain();
      return;
   } else if (result != VK_SUCCESS) {
      assert("failed to present swap chain image");
   }
   mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mNumSwapChainImages;

   mCurrentImageIndex = -1;
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
      glfwExtensions = Window::GetGLFWVulkanExtentensions(&glfwExtensionCount);

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
      ASSERT_RET_FALSE("Failed to create Instance");
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
   VkSurfaceCapabilitiesKHR capabilities{};
   std::vector<VkSurfaceFormatKHR> formats{};
   std::vector<VkPresentModeKHR> presentModes{};
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

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
   for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
         return availableFormat;
      }
   }

   return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
   for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
         return availablePresentMode;
      }
   }

   return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Window* aWindow) {
   ASSERT_VALID(aWindow);
   if (capabilities.currentExtent.width != UINT32_MAX) {
      return capabilities.currentExtent;
   } else {
      VkExtent2D actualExtent = aWindow->GetFBExtent();

      actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
   }
}

bool VulkanManager::CreateSwapchain(Window* aWindow) {
   SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mPhysicalDevice, mSurface);

   VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
   VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
   VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, aWindow);

   uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
   if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
   }

   VkSwapchainCreateInfoKHR createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   createInfo.surface = mSurface;
   createInfo.minImageCount = imageCount;
   createInfo.imageFormat = surfaceFormat.format;
   createInfo.imageExtent = extent;
   createInfo.imageArrayLayers = 1;
   createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

   QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice, mSurface);
   uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

   if (indices.graphicsFamily != indices.presentFamily) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
   } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0; // Optional
      createInfo.pQueueFamilyIndices = nullptr; // Optional
   }

   createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
   createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   createInfo.presentMode = presentMode;
   createInfo.clipped = VK_TRUE;

   createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS) {
      assert("failed to create swap chain");
   }

   vkGetSwapchainImagesKHR(mDevice, mSwapChain, &mNumSwapChainImages, nullptr);
   mSwapChainImages.resize(mNumSwapChainImages);
   mSwapChainImageViews.resize(mNumSwapChainImages);

   vkGetSwapchainImagesKHR(mDevice, mSwapChain, &mNumSwapChainImages, mSwapChainImages.data());

   //create Views
   {
      VkImageViewCreateInfo imageViewInfo{};
      imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageViewInfo.format = surfaceFormat.format;
      imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageViewInfo.subresourceRange.baseArrayLayer = 0;
      imageViewInfo.subresourceRange.layerCount = 1;
      imageViewInfo.subresourceRange.baseMipLevel = 0;
      imageViewInfo.subresourceRange.levelCount = 1;
      for (uint32_t i = 0; i < mNumSwapChainImages; i++) {
         imageViewInfo.image = mSwapChainImages[i];
         vkCreateImageView(mDevice, &imageViewInfo, GetAllocationCallback(), &mSwapChainImageViews[i]);
      }
   }

   mSwapChainImageFormat = surfaceFormat.format;
   mSwapChainExtent = extent;
   return true;
}

bool VulkanManager::CreateCommandPoolBuffers() {
   //pool
   {
      QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(mPhysicalDevice, mSurface);

      VkCommandPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

      if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mGraphicsCommandPool) != VK_SUCCESS) {
         ASSERT_RET_FALSE("failed to create command pool!");
      }
      //setObjName(VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)mGraphicsCommandPool, "Main Command Pool");
   }
   //buffers
   {
      assert(mNumSwapChainImages != 0);
      mCommandBuffers.resize(mNumSwapChainImages);

      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = mGraphicsCommandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = mNumSwapChainImages;

      if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS) {
         assert("failed to allocate command buffers!");
      }

      //for (size_t i = 0; i < mCommandBuffers.size(); i++) {
      //   setObjName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)commandBuffers[i], "Main CommandBuffer Frame:" + std::to_string(i));
      //}
   }

   return true;
}

bool VulkanManager::CreateSyncObjects() {
   mImageAvailableSemaphores.resize(mNumSwapChainImages);
   mRenderFinishedSemaphores.resize(mNumSwapChainImages);
   mInFlightFences.resize(mNumSwapChainImages);
   mImagesInFlight.resize(mNumSwapChainImages, VK_NULL_HANDLE);

   VkSemaphoreCreateInfo semaphoreInfo{};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   VkFenceCreateInfo fenceInfo{};
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

   for (size_t i = 0; i < mNumSwapChainImages; i++) {
      if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS) {
         ASSERT_RET_FALSE("failed to create synchronization objects for a frame!");
      }
      //setObjName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)mImageAvailableSemaphores[i], "imageAvailableSemaphores Frame:" + std::to_string(i));
      //setObjName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)mRenderFinishedSemaphores[i], "renderFinishedSemaphores Frame:" + std::to_string(i));
      //setObjName(VK_OBJECT_TYPE_FENCE, (uint64_t)mInFlightFences[i], "inFlightFences Frame:" + std::to_string(i));
   }
}
