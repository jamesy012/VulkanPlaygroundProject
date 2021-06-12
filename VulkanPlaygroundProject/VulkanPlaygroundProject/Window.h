#pragma once

struct GLFWwindow;

class Window {
public:
   void Create(const int aWidth, const int aHeight, const char* aTitle);
   void Destroy();
   void DestroySurface(VkInstance aInstance);
   void SetWindowUserPtr(void* aPtr);
   void SetResizeCallback();
   void SetFocusCallback();

   bool CreateSurface(VkInstance aInstance);

   bool ShouldClose();
   void WaitEvents();
   void Update();

   const VkSurfaceKHR GetSurface()const {
      return mSurface;
   }
   const VkExtent2D GetFBExtent() const;
   GLFWwindow* GetWindow() const {
      return mWindow;
   }

   const bool IsFocused() const;

   static const char** GetGLFWVulkanExtentensions(uint32_t* aCount);

private:
   GLFWwindow* mWindow = nullptr;
   VkSurfaceKHR mSurface = nullptr;
};

