#pragma once

class Window;

class VulkanManager {
public:
   void Create(Window* aWindow);
   void Destroy(Window* aWindow);

   void RenderStart(uint32_t& aFrameIndex);
   void RenderSubmit(std::vector<VkCommandBuffer> aCommandBuffers);
   void RenderEnd();

   const VkInstance GetInstance() const {
      return mInstance;
   }
private:
   bool CreateInstance();
   bool CreateDevice();
   bool CreateSwapchain(Window* aWindow);
   bool CreateCommandPoolBuffers();
   bool CreateSyncObjects();

   VkInstance mInstance = VK_NULL_HANDLE;
   VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

   VkSurfaceKHR mSurface = VK_NULL_HANDLE;

   VkDevice mDevice = VK_NULL_HANDLE;
   VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;

   VkQueue mGraphicsQueue = VK_NULL_HANDLE;
   VkQueue mPresentQueue = VK_NULL_HANDLE;

   VkPhysicalDeviceProperties mDeviceProperties{};
   VkPhysicalDeviceFeatures mDeviceFeatures{};

   VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
   uint32_t mNumSwapChainImages = 0;
   std::vector<VkImage> mSwapChainImages;
   VkFormat mSwapChainImageFormat;
   VkExtent2D mSwapChainExtent;

   VkCommandPool mGraphicsCommandPool = VK_NULL_HANDLE;
   std::vector<VkCommandBuffer> mCommandBuffers;

   std::vector<VkSemaphore> mImageAvailableSemaphores;
   std::vector<VkSemaphore> mRenderFinishedSemaphores;
   std::vector<VkFence> mInFlightFences;
   std::vector<VkFence> mImagesInFlight;
   uint32_t mCurrentFrameIndex = 0;
   int32_t mCurrentImageIndex = -1;
};

