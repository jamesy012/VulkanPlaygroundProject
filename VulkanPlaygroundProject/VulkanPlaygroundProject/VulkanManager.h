#pragma once

#include "RenderPass.h"
#include "Framebuffer.h"

class Window;

class VulkanManager {
public:
   void Create(Window* aWindow);
   void Destroy(Window* aWindow);

   bool RenderStart(VkCommandBuffer& aBuffer, uint32_t& aFrameIndex);
   void RenderSubmit(std::vector<VkCommandBuffer> aCommandBuffers);
   void RenderEnd();

   const VkInstance GetInstance() const {
      return mInstance;
   }
   const VkExtent2D GetSwapchainExtent() const {
      return mSwapChainExtent;
   }
   const RenderPass* GetPresentRenderPass() const {
      return &mPresentRenderPass;
   }
   const Framebuffer* GetPresentFramebuffer(uint32_t aIndex) const {
      assert(aIndex <= mNumSwapChainImages);
      return &mPresentFramebuffer[aIndex];
   }
private:
   bool CreateInstance();
   bool CreateDevice();
   bool CreateSwapchain(Window* aWindow);
   bool CreateCommandPoolBuffers();
   bool CreateSyncObjects();

   //vulkan
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
   std::vector<VkImageView> mSwapChainImageViews;
   VkFormat mSwapChainImageFormat = VK_FORMAT_UNDEFINED;
   VkExtent2D mSwapChainExtent = {};

   VkCommandPool mGraphicsCommandPool = VK_NULL_HANDLE;
   std::vector<VkCommandBuffer> mCommandBuffers;

   std::vector<VkSemaphore> mImageAvailableSemaphores;
   std::vector<VkSemaphore> mRenderFinishedSemaphores;
   std::vector<VkFence> mInFlightFences;
   std::vector<VkFence> mImagesInFlight;
   uint32_t mCurrentFrameIndex = 0;
   int32_t mCurrentImageIndex = -1;

   //Other
   RenderPass mPresentRenderPass;
   std::vector<Framebuffer> mPresentFramebuffer;
};

