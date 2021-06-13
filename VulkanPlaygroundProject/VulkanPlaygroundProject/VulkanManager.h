#pragma once

#include "RenderPass.h"
#include "Framebuffer.h"

class Window;
class RenderPass;

class VulkanManager {
public:
   void Create(Window* aWindow);
   void Destroy();
   void WaitDevice();

   void Update();

   bool RenderStart(VkCommandBuffer& aBuffer, uint32_t& aFrameIndex);
   void RenderSubmit(std::vector<VkCommandBuffer> aCommandBuffers);
   void RenderEnd();

   void OneTimeCommandBufferStart(VkCommandBuffer& aBuffer);
   void OneTimeCommandBufferEnd(VkCommandBuffer& aBuffer);


   const VkInstance GetInstance() const {
      return mInstance;
   }
   const VkDevice GetDevice() const {
      return mDevice;
   }
   const VkExtent2D GetSwapchainExtent() const {
      return mSwapChainExtent;
   }
   const float GetSwapchainAspect() const {
      return mSwapChainExtent.width / (float)mSwapChainExtent.height;
   }
   const RenderPass* GetPresentRenderPass() const {
      return &mPresentRenderPass;
   }
   const Framebuffer* GetPresentFramebuffer(uint32_t aIndex) const {
      ASSERT_IF(aIndex <= mNumSwapChainImages);
      return &mPresentFramebuffer[aIndex];
   }
   const VkImage GetPresentImage(uint32_t aIndex) const {
      ASSERT_IF(aIndex <= mNumSwapChainImages);
      return mSwapChainImages[aIndex];
   }
   const VmaAllocator& GetAllocator() const {
      return mAllocator;
   } 
   const VkFormat& GetColorFormat() const {
      return mSwapChainImageFormat;
   }
   const bool DidResizeLastFrame() const {
      return mResizedLastRender;
   }
   const VkDeviceSize GetUniformBufferAllignment() const {
      return mDeviceProperties.limits.minUniformBufferOffsetAlignment;
   }
   const VkDescriptorPool GetDescriptorPool() const {
      return mDescriptorPool;
   } 
   const VkSampler GetDefaultSampler() const {
      return mDefaultSampler;
   }
   const uint32_t GetCurrentFrameCounter() const {
      return mCurrentFrameCounter;
   }

   std::function<void(void)> mSizeDependentCreateCallback;
   std::function<void(void)> mSizeDependentDestroyCallback;

private:
   bool CreateInstance();
   bool CreateDevice();
   bool CreateSwapchain();
   bool CreateCommandPoolBuffers();
   bool CreateSyncObjects();
   bool CreateImGui();
   bool CreateImGuiSizeDependent();
   void RenderImGui();

   void SwapchainResized();
   void DestroySizeDependent();
   void CreateSizeDependent();

   //vulkan
   VkInstance mInstance = VK_NULL_HANDLE;
   VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

   VkSurfaceKHR mSurface = VK_NULL_HANDLE;

   VkDevice mDevice = VK_NULL_HANDLE;
   VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;

   struct Queue {
      VkQueue mQueue = VK_NULL_HANDLE;
      uint32_t mFamily = 0;
   };
   Queue mGraphicsQueue{};
   Queue mPresentQueue{};

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
   uint32_t mCurrentFrameCounter = 0;

   //Window
   Window* mWindow;

   //Other
   RenderPass mPresentRenderPass;
   std::vector<Framebuffer> mPresentFramebuffer;
   bool mResizedLastRender = true;
   VkDescriptorPool mDescriptorPool;
   VkSampler mDefaultSampler;

   //Buffers
   VmaAllocator mAllocator;

   //ImGui
   VkDescriptorPool mImGuiDescriptorPool = VK_NULL_HANDLE;
   VkRenderPass mImGuiRenderPass = VK_NULL_HANDLE;
   std::vector<VkFramebuffer> mImGuiFramebuffer;
   std::vector<VkCommandBuffer> mImGuiCommandBuffers;
};

