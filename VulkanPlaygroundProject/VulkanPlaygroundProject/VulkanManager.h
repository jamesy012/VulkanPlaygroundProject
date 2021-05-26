#pragma once

class Window;

class VulkanManager {
public:
   void Create(Window* aWindow);
   void Destroy(Window* aWindow);

   const VkInstance GetInstance() const {
      return mInstance;
   }
private:
   bool CreateInstance();
   bool CreateDevice();

   VkInstance mInstance = VK_NULL_HANDLE;
   VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

   VkSurfaceKHR mSurface = VK_NULL_HANDLE;

   VkDevice mDevice = VK_NULL_HANDLE;
   VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;

   VkQueue mGraphicsQueue = VK_NULL_HANDLE;
   VkQueue mPresentQueue = VK_NULL_HANDLE;

   VkPhysicalDeviceProperties mDeviceProperties;
   VkPhysicalDeviceFeatures mDeviceFeatures;

};

