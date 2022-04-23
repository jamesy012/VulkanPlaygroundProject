#pragma once

#include "Framebuffer.h"
#include "RenderPass.h"

class Window;
class RenderTarget;

#define IMGUI_OWN_FRAMEBUFFER 0

class VulkanManager {
  // Singleton
 private:
  static VulkanManager* _VulkanManager;

 public:
  static inline VulkanManager* Get() { return _VulkanManager; };

  // Implementation
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

  void DebugSetName(VkObjectType aType, uint64_t aObject, std::string aName);
  void DebugMarkerStart(VkCommandBuffer aBuffer, std::string aName,
                        glm::vec4 aColor = glm::zero<glm::vec4>());
  void DebugMarkerEnd(VkCommandBuffer aBuffer);
  void DebugMarkerInsert(VkCommandBuffer aBuffer, std::string aName,
                         glm::vec4 aColor = glm::zero<glm::vec4>());

  const VkInstance GetInstance() const { return mInstance; }
  const VkDevice GetDevice() const { return mDevice; }
  const VkExtent2D GetSwapchainExtent() const { return mSwapChainExtent; }
  const float GetSwapchainAspect() const {
    return mSwapChainExtent.width / (float)mSwapChainExtent.height;
  }
  const RenderPass* GetPresentRenderPass() const { return &mPresentRenderPass; }
  const Framebuffer* GetPresentFramebuffer(uint32_t aIndex) const {
    ASSERT_IF(aIndex <= mNumSwapChainImages);
    return &mPresentFramebuffer[aIndex];
  }
  const VkImage GetPresentImage(uint32_t aIndex) const {
    ASSERT_IF(aIndex <= mNumSwapChainImages);
    return mSwapChainImages[aIndex];
  }
  const VmaAllocator& GetAllocator() const { return mAllocator; }
  const VkFormat& GetColorFormat() const { return mSwapChainImageFormat; }
  const bool DidResizeLastFrame() const { return mResizedLastRender; }
  const VkDeviceSize GetUniformBufferAllignment() const {
    return mDeviceProperties.limits.minUniformBufferOffsetAlignment;
  }
  const VkDescriptorPool GetDescriptorPool() const { return mDescriptorPool; }
  const VkSampler GetDefaultSampler() const { return mDefaultSampler; }
  const VkSampler GetDefaultMirrorSampler() const {
    return mDefaultMirrorSampler;
  }
  const VkSampler GetDefaultClampedSampler() const {
    return mDefaultClampedSampler;
  }
  const uint32_t GetCurrentFrameCounter() const { return mCurrentFrameCounter; }

  const void BlitRenderTargetToBackBuffer(VkCommandBuffer aCommandBuffer,
                                          RenderTarget* aRenderTarget) const;

  std::function<void(void)> mSizeDependentCreateCallback;
  std::function<void(void)> mSizeDependentDestroyCallback;

 private:
  bool CreateInstance();
  bool CreateDevice();
  bool CreateSwapchain();
  bool CreateCommandPoolBuffers();
  bool CreateSyncObjects();
  bool CreateImGui();
#if IMGUI_OWN_FRAMEBUFFER
  bool CreateImGuiSizeDependent();
#endif
  void RenderImGui();

  void SwapchainResized();
  void DestroySizeDependent();
  void CreateSizeDependent();

  // vulkan
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

  // Window
  Window* mWindow;

  // Other
  RenderPass mPresentRenderPass;
  std::vector<Framebuffer> mPresentFramebuffer;
  bool mResizedLastRender = true;
  VkDescriptorPool mDescriptorPool;
  VkSampler mDefaultSampler;
  VkSampler mDefaultMirrorSampler;
  VkSampler mDefaultClampedSampler;

  // Buffers
  VmaAllocator mAllocator;

  // ImGui
  VkDescriptorPool mImGuiDescriptorPool = VK_NULL_HANDLE;
  VkRenderPass mImGuiRenderPass = VK_NULL_HANDLE;
#if IMGUI_OWN_FRAMEBUFFER
  std::vector<VkFramebuffer> mImGuiFramebuffer;
#endif
  std::vector<VkCommandBuffer> mImGuiCommandBuffers;

  // debug
  PFN_vkSetDebugUtilsObjectNameEXT mDebugMarkerSetObjectName;
  PFN_vkCmdBeginDebugUtilsLabelEXT mDebugMarkerBegin;
  PFN_vkCmdEndDebugUtilsLabelEXT mDebugMarkerEnd;
  PFN_vkCmdInsertDebugUtilsLabelEXT mDebugMarkerInsert;
};
