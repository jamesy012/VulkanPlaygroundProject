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

      Draw();
   }
}

void Application::Destroy() {
   mVkManager->Destroy(mWindow);
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}

void Application::Draw() {
   uint32_t frameIndex;
   mVkManager->RenderStart(frameIndex);
   mVkManager->RenderEnd();
}
