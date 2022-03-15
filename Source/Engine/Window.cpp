#include "stdafx.h"
#include "Window.h"

#if WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif APPLE
#include <GLFW/glfw3.h>
#endif

void Window::Create(const int aWidth, const int aHeight, const char* aTitle) {
   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   mWindow = glfwCreateWindow(aWidth, aHeight, aTitle, nullptr, nullptr);
}

void Window::Destroy() {
   glfwDestroyWindow(mWindow);
   mWindow = nullptr;
}

void Window::DestroySurface(VkInstance aInstance) {
   ASSERT_VULKAN_VALUE(aInstance);
   ASSERT_VULKAN_VALUE(mSurface);
   vkDestroySurfaceKHR(aInstance, mSurface, GetAllocationCallback());
}

void Window::SetWindowUserPtr(void* aPtr) {
   ASSERT(aPtr != nullptr);
   glfwSetWindowUserPointer(mWindow, aPtr);
}

bool Window::CreateSurface(VkInstance aInstance) {
   ASSERT_VULKAN_VALUE(aInstance);
   const VkResult result = glfwCreateWindowSurface(aInstance, mWindow, GetAllocationCallback(), &mSurface);
   if (result != VK_SUCCESS) {
      ASSERT_RET_FALSE("Failed to create surface");
   }
   return true;
}

bool Window::ShouldClose() {
   return glfwWindowShouldClose(mWindow);
}

void Window::WaitEvents() {
   return glfwWaitEvents();
}

void Window::Update() {
   glfwPollEvents();
}

const VkExtent2D Window::GetFBExtent() const {
   int width, height;
   glfwGetFramebufferSize(mWindow, &width, &height);

   VkExtent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
   };
   return actualExtent;
}

#if WINDOWS
const void* Window::GetHWND() const {
   return glfwGetWin32Window(mWindow);
}
#endif

const bool Window::IsFocused() const {
   return glfwGetWindowAttrib(mWindow, GLFW_FOCUSED);
}

const bool Window::IsHovered() const {
   return glfwGetWindowAttrib(mWindow, GLFW_HOVERED);
}

const char** Window::GetGLFWVulkanExtentensions(uint32_t* aCount) {
   return glfwGetRequiredInstanceExtensions(aCount);
}
