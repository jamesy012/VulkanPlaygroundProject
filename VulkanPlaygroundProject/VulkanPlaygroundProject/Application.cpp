#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "VulkanManager.h"
#include "Window.h"

#include "Framebuffer.h"
#include "RenderPass.h"

void Application::Start() {
   mWindow = new Window();
   mWindow->Create(800, 600, "vulkan");
   mVkManager = new VulkanManager();
   mVkManager->Create(mWindow);
}

void Application::Run() {
   while (!mWindow->ShouldClose()) {
      mWindow->Update();
      mVkManager->Update();

      ImGui::ShowDemoWindow(0);

      Draw();
   }
}

void Application::Destroy() {
   mVkManager->Destroy();
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}

void Application::Draw() {
   uint32_t frameIndex;
   VkCommandBuffer buffer;
   if (mVkManager->RenderStart(buffer, frameIndex) == false) {
      ASSERT_RET();
   }
   static uint32_t frameCounter = 0;
   frameCounter++;

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = 0; // Optional
   beginInfo.pInheritanceInfo = nullptr; // Optional

   VkRenderPassBeginInfo renderBegin{};
   renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderBegin.renderArea.extent = mVkManager->GetSwapchainExtent();
   renderBegin.renderPass = mVkManager->GetPresentRenderPass()->GetRenderPass();
   VkClearValue clearColor{};
   clearColor.color.float32[0] = abs(sin((frameCounter*0.5f)/5000.0f));
   clearColor.color.float32[1] = abs(sin((frameCounter*0.2f)/5000.0f));
   clearColor.color.float32[2] = abs(sin((frameCounter*0.1f)/5000.0f));
   clearColor.color.float32[3] = 1.0f;

   renderBegin.clearValueCount = 1;
   renderBegin.pClearValues = &clearColor;
   renderBegin.framebuffer = mVkManager->GetPresentFramebuffer(frameIndex)->GetFramebuffer();
   vkBeginCommandBuffer(buffer, &beginInfo);

   vkCmdBeginRenderPass(buffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdEndRenderPass(buffer);
   vkEndCommandBuffer(buffer);


   mVkManager->RenderEnd();
}
