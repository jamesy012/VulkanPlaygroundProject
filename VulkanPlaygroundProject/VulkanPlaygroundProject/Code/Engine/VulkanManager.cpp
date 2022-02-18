#include "stdafx.h"
#include "VulkanManager.h"

#include <optional>
#include <set>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vulkan.h>

#if USE_VR == 1
#include <openvr.h>
#endif

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "Window.h"

#include "RenderTarget.h"
#include "RenderManager.h"

VulkanManager* VulkanManager::_VulkanManager = nullptr;

RenderManager* gRenderManager = nullptr;

//#define VULKAN_API_VERSION VK_HEADER_VERSION_COMPLETE
#define VULKAN_API_VERSION VK_API_VERSION_1_2

#define MAX_DESCRIPTOR_SETS 50
constexpr VkDescriptorPoolSize poolSizes[3] = { 
   { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX_DESCRIPTOR_SETS },
   { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DESCRIPTOR_SETS },
   { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DESCRIPTOR_SETS }
};

const std::vector<const char*> validationLayers = {
   "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> constantDeviceExtensions = {
   VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

std::vector<void*> delayedDelete;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT messageType,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   void* pUserData) {

   static const std::vector<int32_t> skipMessages = { 416909302, 1303270965 };
   for (int i = 0; i < skipMessages.size(); i++) {
       if (pCallbackData->messageIdNumber == skipMessages[i]) {
           return VK_FALSE;
       }
   }

   LOG("VULKAN %s: \n\t %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);

   if (pCallbackData->messageIdNumber != 0) {
      ASSERT("Vulkan Error");
   }

   return VK_FALSE;
}

#if USE_VR == 1
vr::IVRSystem* m_pHMD;
vr::IVRRenderModels* m_pRenderModels;
vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];


std::string GetTrackedDeviceString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL)
{
    uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
    if (unRequiredBufferLen == 0)
        return "";

    char* pchBuffer = new char[unRequiredBufferLen];
    unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void ProcessVREvent(const vr::VREvent_t& event)
{
    switch (event.eventType)
    {
    case vr::VREvent_TrackedDeviceDeactivated:
    {
        LOG("Device %u detached.\n", event.trackedDeviceIndex);
    }
    break;
    case vr::VREvent_TrackedDeviceUpdated:
    {
        LOG("Device %u updated.\n", event.trackedDeviceIndex);
    }
    break;
    case vr::VREvent_ButtonPress:
    {
        LOG("Button Press %u.\n", event.data.controller.button);
    }
    break;
    }
}
#endif

void VulkanManager::Create(Window* aWindow) {
   ASSERT_IF( _VulkanManager == nullptr );
   _VulkanManager = this;

   ASSERT_IF( gRenderManager == nullptr );
   gRenderManager = new RenderManager();

#if USE_VR == 1
   //VR Test
   {
       vr::EVRInitError eError = vr::VRInitError_None;
       m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
       if (eError != vr::VRInitError_None)
       {
           m_pHMD = NULL;
           LOG("Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
           return;
       }

       m_pRenderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
       if (!m_pRenderModels)
       {
           m_pHMD = NULL;
           vr::VR_Shutdown();

           LOG("Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
           return;
       }

       std::string m_strDriver = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ManufacturerName_String);
       std::string m_strDisplay = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_RenderModelName_String);
       LOG("VR Startup %s - %s\n", m_strDriver.c_str(), m_strDisplay.c_str());
   }
#endif

   mWindow = aWindow;
   CreateInstance();
   if (aWindow->CreateSurface(GetInstance())) {
      mSurface = aWindow->GetSurface();
   }

   CreateDevice();

   CreateSwapchain();

   CreateCommandPoolBuffers();

   CreateSyncObjects();

   CreateImGui();

   //vma
   {
      VmaAllocatorCreateInfo createInfo{};
      createInfo.vulkanApiVersion = VULKAN_API_VERSION;
      createInfo.device = mDevice;
      createInfo.instance = mInstance;
      createInfo.physicalDevice = mPhysicalDevice;
      createInfo.pAllocationCallbacks = GetAllocationCallback();

      vmaCreateAllocator(&createInfo, &mAllocator);
   }

   {
      mPresentRenderPass.Create(mDevice, mSwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
   }
   CreateSizeDependent();

   //descriptor
   {
      VkDescriptorPoolCreateInfo poolCreate{};
      poolCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolCreate.maxSets = MAX_DESCRIPTOR_SETS;
      poolCreate.poolSizeCount = SIZEOF_ARRAY(poolSizes);
      poolCreate.pPoolSizes = poolSizes;
      vkCreateDescriptorPool(GetDevice(), &poolCreate, GetAllocationCallback(), &mDescriptorPool);
   }

   {
      VkSamplerCreateInfo samplerInfo{};
      samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      samplerInfo.magFilter = VK_FILTER_LINEAR;
      samplerInfo.minFilter = VK_FILTER_LINEAR;
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      samplerInfo.anisotropyEnable = VK_TRUE;

      VkPhysicalDeviceProperties properties{};
      samplerInfo.maxAnisotropy = mDeviceProperties.limits.maxSamplerAnisotropy;
      samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
      samplerInfo.unnormalizedCoordinates = VK_FALSE;
      samplerInfo.compareEnable = VK_FALSE;
      samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
      samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      samplerInfo.mipLodBias = 0.0f;
      samplerInfo.minLod = 0.0f;
      samplerInfo.maxLod = 0.0f;

      if (vkCreateSampler(GetDevice(), &samplerInfo, nullptr, &mDefaultSampler) != VK_SUCCESS) {
         throw std::runtime_error("failed to create texture gltfSampler!");
      }  
      samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      if (vkCreateSampler(GetDevice(), &samplerInfo, nullptr, &mDefaultMirrorSampler) != VK_SUCCESS) {
         throw std::runtime_error("failed to create texture gltfSampler!");
      }
      samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      if (vkCreateSampler(GetDevice(), &samplerInfo, nullptr, &mDefaultClampedSampler) != VK_SUCCESS) {
         throw std::runtime_error("failed to create texture gltfSampler!");
      }
   }

   //Pre first run setup
   {
      //setup managers
      gRenderManager->StartUp();

      //setup anything that needs to be in a command buffer
      VkCommandBuffer buffer;
      OneTimeCommandBufferStart(buffer);
      {
         //imgui text, debug only?
         ImGui_ImplVulkan_CreateFontsTexture(buffer);
      }
      OneTimeCommandBufferEnd(buffer);
   }

   for (int i = 0; i < delayedDelete.size(); i++) {
       delete delayedDelete[i];
   }
   delayedDelete.clear();
}

void VulkanManager::Destroy() {
   WaitDevice();

#if USE_VR == 1
   if (m_pHMD) {
       m_pHMD = NULL;
       vr::VR_Shutdown();
   }
#endif

   vkDestroySampler(GetDevice(), mDefaultSampler, GetAllocationCallback());
   vkDestroySampler(GetDevice(), mDefaultMirrorSampler, GetAllocationCallback());
   vkDestroySampler(GetDevice(), mDefaultClampedSampler, GetAllocationCallback());
   vkDestroyDescriptorPool(GetDevice(), mDescriptorPool, GetAllocationCallback());

   //ImGui
   {
      ImGui_ImplVulkan_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();

      vkFreeCommandBuffers(mDevice, mGraphicsCommandPool, mNumSwapChainImages, mImGuiCommandBuffers.data());
      mImGuiCommandBuffers.clear();

      vkDestroyRenderPass(mDevice, mImGuiRenderPass, GetAllocationCallback());
      mImGuiRenderPass = VK_NULL_HANDLE;
      vkDestroyDescriptorPool(mDevice, mImGuiDescriptorPool, GetAllocationCallback());
      mImGuiDescriptorPool = VK_NULL_HANDLE;
   }


   mPresentRenderPass.Destroy(mDevice);

   for (size_t i = 0; i < mNumSwapChainImages; i++) {
      vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
      vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
   }
   mRenderFinishedSemaphores.clear();
   mImageAvailableSemaphores.clear();
   mInFlightFences.clear();

   DestroySizeDependent();

   vkFreeCommandBuffers(mDevice, mGraphicsCommandPool, static_cast<uint32_t>(mCommandBuffers.size()), mCommandBuffers.data());
   mCommandBuffers.clear();
   vkDestroyCommandPool(mDevice, mGraphicsCommandPool, GetAllocationCallback());
   mGraphicsCommandPool = VK_NULL_HANDLE;

   ASSERT_IF( gRenderManager != nullptr );
   delete gRenderManager;
   gRenderManager = nullptr;

   vmaDestroyAllocator(mAllocator);

   vkDestroyDevice(mDevice, CreateAllocationCallbacks());
   mDevice = VK_NULL_HANDLE;

   mWindow->DestroySurface(GetInstance());

   if (enableValidationLayers && mDebugMessenger != VK_NULL_HANDLE) {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
         func(mInstance, mDebugMessenger, GetAllocationCallback());
      }
   }

   vkDestroyInstance(mInstance, GetAllocationCallback());

   ASSERT_IF( _VulkanManager != nullptr );
   _VulkanManager = nullptr;
}

void VulkanManager::WaitDevice() {
   vkDeviceWaitIdle(mDevice);
}

void VulkanManager::Update() {
   {
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      //wrap mouse around screen, keeping imgui happy
      {
         ImGuiIO& io = ImGui::GetIO();
         //needs to happen for two frames in a row for imgui to not mess up values
         static bool frameDelayMouseDeltaReset = false;
         if (frameDelayMouseDeltaReset) {
            io.MouseDelta = ImVec2(0, 0);
            frameDelayMouseDeltaReset = false;
         }
         if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            static POINT p = { 0, 0 };
            static POINT pRel = { 0, 0 };
            if (GetCursorPos(&p)) {
               pRel = p;
               if (ScreenToClient((HWND)mWindow->GetHWND(), &pRel)) {
                  VkExtent2D size = mWindow->GetFBExtent();
                  bool move = false;
                  if (pRel.x < 2) {
                     p.x += size.width - 5;
                     move = true;
                  } else if ((uint32_t)pRel.x > size.width - 2) {
                     p.x -= size.width - 5u;
                     move = true;
                  }
                  if (pRel.y < 2) {
                     p.y += size.height - 5;
                     move = true;
                  } else if ((uint32_t)pRel.y > size.height - 2) {
                     p.y -= size.height - 5u;
                     move = true;
                  }

                  if (move) {
                     SetCursorPos(p.x, p.y);
                     io.MouseDelta = ImVec2(0, 0);
                     frameDelayMouseDeltaReset = true;
                  }
               }
            }
         }
      }
   }

#if USE_VR == 1
   //VR
   {
       vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
       if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
           vr::HmdMatrix34_t mat = m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
           mHmdDevicePos = glm::mat4(
               mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
               mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
               mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
               mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f);
           //mHmdDevicePos = glm::mat4(
           //    mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
           //    mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
           //    mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
           //    0.0f,               0.0f,       0.0f,       1.0f);
           mHmdDevicePos = glm::inverse(mHmdDevicePos);
           //mHmdDevicePos = glm::mat4(glm::make_mat3x4(m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m));
       }
       if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd + 1].bPoseIsValid) {
           vr::HmdMatrix34_t mat = m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd + 1].mDeviceToAbsoluteTracking;
           mVrControllerPos[0] = glm::mat4(
               mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
               mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
               mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
               mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f);
           //mVrControllerPos[0] = glm::inverse(mVrControllerPos[0]);
       }

       // Process SteamVR events
       vr::VREvent_t event;
       while (m_pHMD->PollNextEvent(&event, sizeof(event)))
       {
           ProcessVREvent(event);
       }

   }
#endif
}

bool VulkanManager::RenderStart(VkCommandBuffer& aBuffer, uint32_t& aFrameIndex) {
   mCurrentFrameCounter++;

   ASSERT_IF(mCurrentImageIndex == -1);
   VkResult result = vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrameIndex], VK_NULL_HANDLE, &aFrameIndex);

   if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      SwapchainResized();
      return false;
   } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      ASSERT("failed to acqure swap chain image");
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

void VulkanManager::SubmitVrEye(VkCommandBuffer aCommandBuffer, RenderTarget* aRenderTarget, bool aRightEye) {
#if USE_VR == 1
    // Submit to SteamVR
    vr::VRTextureBounds_t bounds;
    bounds.uMin = 0.0f;
    bounds.uMax = 1.0f;
    bounds.vMin = 0.0f;
    bounds.vMax = 1.0f;

    vr::VRVulkanTextureData_t vulkanData;
    //vulkanData.m_nImage = (uint64_t)m_leftEyeDesc.m_pImage;
    vulkanData.m_nImage = (uint64_t)aRenderTarget->GetColorImage();
    vulkanData.m_pDevice = (VkDevice_T*)mDevice;
    vulkanData.m_pPhysicalDevice = (VkPhysicalDevice_T*)mPhysicalDevice;
    vulkanData.m_pInstance = (VkInstance_T*)instance;
    vulkanData.m_pQueue = (VkQueue_T*)mGraphicsQueue.mQueue;
    vulkanData.m_nQueueFamilyIndex = mGraphicsQueue.mFamily;

    vulkanData.m_nWidth = mSwapChainExtent.width;
    vulkanData.m_nHeight = mSwapChainExtent.height;
    //vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_SRGB;
    vulkanData.m_nFormat = GetColorFormat();
    vulkanData.m_nSampleCount = 1;

    vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
    vr::EVRCompositorError errorLeft = vr::VRCompositor()->Submit(vr::Eye_Left, &texture, &bounds);
    ////vulkanData.m_nImage = (uint64_t)m_rightEyeDesc.m_pImage;
    vr::EVRCompositorError errorRight = vr::VRCompositor()->Submit(vr::Eye_Right, &texture, &bounds);
#endif
}

void VulkanManager::RenderSubmit(std::vector<VkCommandBuffer> aCommandBuffers) {

   RenderImGui();

   ASSERT_IF(mCurrentImageIndex != -1);
   VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrameIndex] };

   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrameIndex] };
   VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;

   VkCommandBuffer commandBuffersList[2] = { mCommandBuffers[mCurrentImageIndex], mImGuiCommandBuffers[mCurrentImageIndex] };
   submitInfo.commandBufferCount = sizeof(commandBuffersList) / sizeof(commandBuffersList[0]);
   submitInfo.pCommandBuffers = commandBuffersList;


   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = signalSemaphores;

   //vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
   vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrameIndex]);

   if (vkQueueSubmit(mGraphicsQueue.mQueue, 1, &submitInfo, mInFlightFences[mCurrentFrameIndex]) != VK_SUCCESS) {
      ASSERT("failed to submit draw command buffer!");
   }
}

void VulkanManager::RenderEnd() {
   ASSERT_IF(mCurrentImageIndex != -1);
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
      vkQueueWaitIdle(mPresentQueue.mQueue);
   }

   VkResult result = vkQueuePresentKHR(mPresentQueue.mQueue, &presentInfo);

   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR /*|| framebufferResized*/) {
      SwapchainResized();
      return;
   } else if (result != VK_SUCCESS) {
      ASSERT("failed to present swap chain image");
   }
   mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mNumSwapChainImages;

   mCurrentImageIndex = -1;
   mResizedLastRender = false;
}

void VulkanManager::OneTimeCommandBufferStart(VkCommandBuffer& aBuffer) {
   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = mGraphicsCommandPool;
   allocInfo.commandBufferCount = 1;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   vkAllocateCommandBuffers(mDevice, &allocInfo, &aBuffer);

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   vkBeginCommandBuffer(aBuffer, &beginInfo);
}

void VulkanManager::OneTimeCommandBufferEnd(VkCommandBuffer& aBuffer) {
   vkEndCommandBuffer(aBuffer);
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &aBuffer;

   vkQueueSubmit(mGraphicsQueue.mQueue, 1, &submitInfo, VK_NULL_HANDLE);
   vkQueueWaitIdle(mGraphicsQueue.mQueue);

   vkFreeCommandBuffers(mDevice, mGraphicsCommandPool, 1, &aBuffer);
}

void VulkanManager::DebugSetName(VkObjectType aType, uint64_t aObject, std::string aName) {
   if (mDebugMarkerSetObjectName) {
      VkDebugUtilsObjectNameInfoEXT info{};
      info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
      info.objectType = aType;
      info.objectHandle = aObject;
      info.pObjectName = aName.c_str();
      VkResult result = mDebugMarkerSetObjectName(GetDevice(), &info);
      ASSERT_VULKAN_SUCCESS(result);
   }
}

void VulkanManager::DebugMarkerStart(VkCommandBuffer aBuffer, std::string aName, glm::vec4 aColor) {
   if (mDebugMarkerBegin) {
      VkDebugUtilsLabelEXT info{};
      info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
      memcpy(info.color, glm::value_ptr(aColor), sizeof(float) * 4);
      info.pLabelName = aName.c_str();
      mDebugMarkerBegin(aBuffer, &info);
   }
}

void VulkanManager::DebugMarkerEnd(VkCommandBuffer aBuffer) {
   if (mDebugMarkerEnd) {
      mDebugMarkerEnd(aBuffer);
   }
}

void VulkanManager::DebugMarkerInsert(VkCommandBuffer aBuffer, std::string aName, glm::vec4 aColor) {
   if (mDebugMarkerInsert) {
      VkDebugUtilsLabelEXT info{};
      info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
      memcpy(info.color, glm::value_ptr(aColor), sizeof(float) * 4);
      info.pLabelName = aName.c_str();
      mDebugMarkerInsert(aBuffer, &info);
   }
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

const void VulkanManager::BlitRenderTargetToBackBuffer(VkCommandBuffer aCommandBuffer, RenderTarget* aRenderTarget) const {
   VulkanManager::Get()->DebugMarkerStart(aCommandBuffer, "Blit RT to Backbuffer");

   const VkImage presentImage = GetPresentImage(mCurrentFrameIndex);

   SetImageLayout(aCommandBuffer, presentImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   VkImageBlit blit{};
   //[0] being 0,0 to get the full image
   blit.srcOffsets[1].x = aRenderTarget->GetSize().width;
   blit.srcOffsets[1].y = aRenderTarget->GetSize().height;
   blit.srcOffsets[1].z = 1;
   blit.dstOffsets[1].x = GetSwapchainExtent().width;
   blit.dstOffsets[1].y = GetSwapchainExtent().height;
   blit.dstOffsets[1].z = 1;
   blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.srcSubresource.layerCount = 1;
   blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.dstSubresource.layerCount = 1;
   vkCmdBlitImage(aCommandBuffer, aRenderTarget->GetColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, presentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);
   SetImageLayout(aCommandBuffer, presentImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
   
   VulkanManager::Get()->DebugMarkerEnd(aCommandBuffer);
}

bool VulkanManager::CreateInstance() {
   {
      LOG("Loading Vulkan sdk: ");
      uint32_t version = VK_HEADER_VERSION_COMPLETE;
      LOG("Version: %i.%i.%i\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
   }

   if (enableValidationLayers && !CheckVkLayerSupport(validationLayers)) {
      ASSERT_RET_FALSE("validation layers requested, but not available!");
   }

   VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
   debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugCreateInfo.pfnUserCallback = DebugCallback;
   debugCreateInfo.pNext = nullptr;

   VkApplicationInfo appInfo = {};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Hello Triangle";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "No Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   //appInfo.apiVersion = VK_API_VERSION_1_0;
   appInfo.apiVersion = VULKAN_API_VERSION;

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
#if USE_VR == 1
   {
       char* vrExtenstionString;
       uint32_t vrExtenstionSize = 0;
       vrExtenstionSize = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(nullptr, 0);
       vrExtenstionString = new char[vrExtenstionSize];
       delayedDelete.push_back(vrExtenstionString);
       vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(vrExtenstionString, vrExtenstionSize);
       int lastSplit = 0;
       for (int i = 0; i < vrExtenstionSize; i++) {
           if (vrExtenstionString[i] == ' ') {
               vrExtenstionString[i] = 0;
               extensions.push_back(vrExtenstionString + lastSplit);
               lastSplit = i+1;
           }
       }
       extensions.push_back(vrExtenstionString + lastSplit);
   }
#endif

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

   //vkCreateInstance result check
   if (result != VK_SUCCESS) {
      ASSERT_RET_FALSE("Failed to create Instance");
   }

   if (enableValidationLayers) {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
      if (func != nullptr) {
         func(mInstance, &debugCreateInfo, GetAllocationCallback(), &mDebugMessenger);
      }
   }

   mDebugMarkerSetObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(mInstance, "vkSetDebugUtilsObjectNameEXT");
   mDebugMarkerBegin = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(mInstance, "vkCmdBeginDebugUtilsLabelEXT");
   mDebugMarkerEnd = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(mInstance, "vkCmdEndDebugUtilsLabelEXT");
   mDebugMarkerInsert = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(mInstance, "vkCmdInsertDebugUtilsLabelEXT");

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

bool CheckDeviceExpensionSupport(VkPhysicalDevice aDevice, std::vector<const char*> aDeviceExtensions) {
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, nullptr);

   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, availableExtensions.data());

   std::set<std::string> requiredExtensions(aDeviceExtensions.begin(), aDeviceExtensions.end());

   for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
   }

   return requiredExtensions.empty();
}

bool IsDeviceSuitable(VkPhysicalDevice aDevice, VkSurfaceKHR aSurface, std::vector<const char*> aDeviceExtensions) {
   //VkPhysicalDeviceProperties deviceProperties;
   //vkGetPhysicalDeviceProperties(device, &deviceProperties);
   //VkPhysicalDeviceFeatures deviceFeatures;
   //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
   //
   //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
   //   deviceFeatures.geometryShader;

   QueueFamilyIndices indices = FindQueueFamilies(aDevice, aSurface);

   bool extensionsSupported = CheckDeviceExpensionSupport(aDevice, aDeviceExtensions);

   bool swapChainAdequte = false;
   if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(aDevice, aSurface);
      swapChainAdequte = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
   }

   VkPhysicalDeviceFeatures supportedFeatures;
   vkGetPhysicalDeviceFeatures(aDevice, &supportedFeatures);

   return indices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequte && supportedFeatures.samplerAnisotropy;
}

void GetDeviceExtensions(VkPhysicalDevice aDevice, std::vector<const char*>& aExtensions) {
    aExtensions = constantDeviceExtensions;

#if USE_VR == 1
    {
        char* vrDeviceExtenstionString;
        uint32_t vrExtenstionSize = 0;
        vrExtenstionSize = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(aDevice, nullptr, 0);
        vrDeviceExtenstionString = new char[vrExtenstionSize];
        delayedDelete.push_back(vrDeviceExtenstionString);
        vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(aDevice, vrDeviceExtenstionString, vrExtenstionSize);
        int lastSplit = 0;
        for (int i = 0; i < vrExtenstionSize; i++) {
            if (vrDeviceExtenstionString[i] == ' ') {
                vrDeviceExtenstionString[i] = 0;
                aExtensions.push_back(vrDeviceExtenstionString + lastSplit);
                lastSplit = i + 1;
            }
        }
        aExtensions.push_back(vrDeviceExtenstionString + lastSplit);
    }
#endif
}

bool VulkanManager::CreateDevice() {
   ASSERT_VULKAN_VALUE(mInstance);

#if USE_VR == 1
   //uint64_t pHMDPhysicalDevice = 0;
   //m_pHMD->GetOutputDevice(&pHMDPhysicalDevice, vr::TextureType_Vulkan, (VkInstance_T*)mInstance);
#endif

   std::vector<const char*> deviceExtensions;

   //physical device
   {
      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
      if (deviceCount == 0) {
         ASSERT_RET_FALSE("failed to find GPUs with vulkan support");
      }

      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

      for (const VkPhysicalDevice& device : devices) {
          GetDeviceExtensions(device, deviceExtensions);
         if (IsDeviceSuitable(device, mSurface, deviceExtensions)) {
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
      deviceFeatures.multiDrawIndirect = VK_TRUE;
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

      vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue.mQueue);
      vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue.mQueue);
      mGraphicsQueue.mFamily = indices.graphicsFamily.value();
      mPresentQueue.mFamily = indices.presentFamily.value();
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

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window* aWindow) {
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

bool VulkanManager::CreateSwapchain() {
   SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mPhysicalDevice, mSurface);

   VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
   VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
   VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, mWindow);

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
   createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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
      ASSERT("failed to create swap chain");
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
      DebugSetObjName(VK_OBJECT_TYPE_COMMAND_POOL, mGraphicsCommandPool, "Main Command Pool");
   }
   //buffers
   {
      ASSERT_IF(mNumSwapChainImages != 0);
      mCommandBuffers.resize(mNumSwapChainImages);

      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = mGraphicsCommandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = mNumSwapChainImages;

      if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS) {
         ASSERT("failed to allocate command buffers!");
      }

      for (size_t i = 0; i < mCommandBuffers.size(); i++) {
         DebugSetObjName(VK_OBJECT_TYPE_COMMAND_BUFFER, mCommandBuffers[i], "Main CommandBuffer Frame:" + std::to_string(i));
      }
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
   return true;
}

bool VulkanManager::CreateImGui() {

   // Create Descriptor Pool
   {
      VkDescriptorPoolSize pool_sizes[] =
      {
         { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
         { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
         { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
         { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
         { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
      };
      VkDescriptorPoolCreateInfo pool_info = {};
      pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
      pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
      pool_info.pPoolSizes = pool_sizes;
      VkResult result = vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mImGuiDescriptorPool);
      ASSERT_VULKAN_SUCCESS_RET_FALSE(result);
      //setObjName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)imguiDescriptorPool, "ImGUI Descritor Pool");
   }

   // Create the Render Pass
   {
      VkAttachmentDescription attachment = {};
      attachment.format = mSwapChainImageFormat;
      attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      //attachment.loadOp = wd->ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      VkAttachmentReference color_attachment = {};
      color_attachment.attachment = 0;
      color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &color_attachment;
      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      VkRenderPassCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      info.attachmentCount = 1;
      info.pAttachments = &attachment;
      info.subpassCount = 1;
      info.pSubpasses = &subpass;
      info.dependencyCount = 1;
      info.pDependencies = &dependency;
      VkResult result = vkCreateRenderPass(mDevice, &info, nullptr, &mImGuiRenderPass);
      ASSERT_VULKAN_SUCCESS_RET_FALSE(result);
      //setObjName(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)wd->RenderPass, "ImGui Render Pass");

      // We do not create a pipeline by default as this is also used by examples' main.cpp,
      // but secondary viewport in multi-viewport mode may want to create one with:
      //ImGui_ImplVulkan_CreatePipeline(device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline, g_Subpass);
   }

   CreateImGuiSizeDependent();

   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
   io.ConfigWindowsMoveFromTitleBarOnly = true;
   // Setup Dear ImGui style
   //ImGui::StyleColorsDark();
   ImGui::StyleColorsClassic();

   // Setup Platform/Renderer backends
   ImGui_ImplGlfw_InitForVulkan(mWindow->GetWindow(), true);
   ImGui_ImplVulkan_InitInfo init_info = {};
   init_info.Instance = mInstance;
   init_info.PhysicalDevice = mPhysicalDevice;
   init_info.Device = mDevice;
   init_info.QueueFamily = mGraphicsQueue.mFamily;
   init_info.Queue = mGraphicsQueue.mQueue;
   init_info.PipelineCache = VK_NULL_HANDLE;
   init_info.DescriptorPool = mImGuiDescriptorPool;
   init_info.Allocator = nullptr;
   init_info.MinImageCount = mNumSwapChainImages;
   init_info.ImageCount = mNumSwapChainImages;
   init_info.CheckVkResultFn = CheckVulkanResult;
   ImGui_ImplVulkan_Init(&init_info, mImGuiRenderPass);

   mImGuiCommandBuffers.resize(mNumSwapChainImages);

   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = mGraphicsCommandPool;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = (uint32_t)mImGuiCommandBuffers.size();

   if (vkAllocateCommandBuffers(mDevice, &allocInfo, mImGuiCommandBuffers.data()) != VK_SUCCESS) {
      ASSERT_RET_FALSE("failed to allocate command buffers!");
   }
   for (size_t i = 0; i < mImGuiCommandBuffers.size(); i++) {
      DebugSetObjName(VK_OBJECT_TYPE_COMMAND_BUFFER, mImGuiCommandBuffers[i], "ImGui Command Buffers Buffers Frame:" + std::to_string(i));
   }

   return true;
}

bool VulkanManager::CreateImGuiSizeDependent() {
   mImGuiFramebuffer.resize(mNumSwapChainImages);

   // Create Framebuffer
   {
      VkImageView attachment[1];
      VkFramebufferCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      info.renderPass = mImGuiRenderPass;
      info.attachmentCount = 1;
      info.pAttachments = attachment;
      info.width = mSwapChainExtent.width;
      info.height = mSwapChainExtent.height;
      info.layers = 1;
      for (uint32_t i = 0; i < mNumSwapChainImages; i++) {
         attachment[0] = mSwapChainImageViews[i];
         VkResult result = vkCreateFramebuffer(mDevice, &info, nullptr, &mImGuiFramebuffer[i]);
         ASSERT_VULKAN_SUCCESS_RET_FALSE(result);
         //setObjName(VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)imguiFramebuffers[i], "ImGui Frame Buffers Frame:" + std::to_string(i));
      }
   }
   return true;
}

void VulkanManager::RenderImGui() {
   PROFILE_START_SCOPED("ImGui Render");
   ImGui::Render();
   ImDrawData* draw_data = ImGui::GetDrawData();
   {
      VkResult result;
      {
         VkCommandBufferBeginInfo bufferBeginInfo = {};
         bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
         bufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
         result = vkBeginCommandBuffer(mImGuiCommandBuffers[mCurrentImageIndex], &bufferBeginInfo);
         ASSERT_VULKAN_SUCCESS(result);
         VulkanManager::Get()->DebugMarkerStart(mImGuiCommandBuffers[mCurrentImageIndex], "ImGui Render");

         VkRenderPassBeginInfo renderBeginInfo = {};
         renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
         renderBeginInfo.renderPass = mImGuiRenderPass;
         renderBeginInfo.framebuffer = mImGuiFramebuffer[mCurrentImageIndex];
         //renderBeginInfo.framebuffer = mPresentFramebuffer[mCurrentImageIndex].GetFramebuffer();
         renderBeginInfo.renderArea.offset = { 0, 0 };
         renderBeginInfo.renderArea.extent = mSwapChainExtent;
         renderBeginInfo.clearValueCount = 1;
         VkClearValue clearColor{};
         renderBeginInfo.pClearValues = &clearColor;
         vkCmdBeginRenderPass(mImGuiCommandBuffers[mCurrentImageIndex], &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

         // Record dear imgui primitives into command buffer
         ImGui_ImplVulkan_RenderDrawData(draw_data, mImGuiCommandBuffers[mCurrentImageIndex]);

         // Submit command buffer
         vkCmdEndRenderPass(mImGuiCommandBuffers[mCurrentImageIndex]);

         VulkanManager::Get()->DebugMarkerEnd(mImGuiCommandBuffers[mCurrentImageIndex]);
         result = vkEndCommandBuffer(mImGuiCommandBuffers[mCurrentImageIndex]);
         ASSERT_VULKAN_SUCCESS(result);
      }
   }
}

void VulkanManager::SwapchainResized() {
   VkExtent2D size = mWindow->GetFBExtent();
   while (size.width == 0 || size.height == 0) {
      size = mWindow->GetFBExtent();
      mWindow->WaitEvents();
   }
   vkDeviceWaitIdle(mDevice);
   mCurrentImageIndex = -1;
   DestroySizeDependent();
   CreateSizeDependent();
   mResizedLastRender = true;
}

void VulkanManager::DestroySizeDependent() {

   if (mSizeDependentDestroyCallback) {
      mSizeDependentDestroyCallback();
   }

   for (uint32_t i = 0; i < mNumSwapChainImages; i++) {
      mPresentFramebuffer[i].Destroy(mDevice);
      vkDestroyFramebuffer(mDevice, mImGuiFramebuffer[i], GetAllocationCallback());
      vkDestroyImageView(mDevice, mSwapChainImageViews[i], GetAllocationCallback());
   }

   mPresentFramebuffer.clear();
   mImGuiFramebuffer.clear();
   mSwapChainImageViews.clear();

   vkDestroySwapchainKHR(mDevice, mSwapChain, GetAllocationCallback());
   mSwapChain = VK_NULL_HANDLE;
   mSwapChainImages.clear();
   mNumSwapChainImages = 0;
   mSwapChainExtent = {};
   mSwapChainImageFormat = VK_FORMAT_UNDEFINED;

}

void VulkanManager::CreateSizeDependent() {
   if (mSwapChain == VK_NULL_HANDLE) {
      CreateSwapchain();
   }
   if (mImGuiFramebuffer.size() == 0) {
      CreateImGuiSizeDependent();
   }
   if (mPresentFramebuffer.size() == 0) {
      mPresentFramebuffer.resize(mNumSwapChainImages);
      for (uint32_t i = 0; i < mNumSwapChainImages; i++) {
         std::vector<VkImageView> view = { mSwapChainImageViews[i] };
         mPresentFramebuffer[i].Create(mDevice, mSwapChainExtent, &mPresentRenderPass, view);
      }
   }

   if (mSizeDependentCreateCallback) {
      mSizeDependentCreateCallback();
   }
}
