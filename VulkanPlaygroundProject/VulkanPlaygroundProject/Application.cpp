#include "stdafx.h"
#include "Application.h"

#include "VulkanManager.h"
#include "Window.h"

void Application::Start() {
   mWindow = new Window();
   mWindow->Create(800, 600, "vulkan");
   mVkManager = new VulkanManager();
   mVkManager->Create(mWindow);
}

void Application::Run() {
   while (!mWindow->ShouldClose()) {
      mWindow->Update();
   }
}

void Application::Destroy() {
   mVkManager->Destroy(mWindow);
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}
