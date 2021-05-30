#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "VulkanManager.h"
#include "Window.h"

#include "Framebuffer.h"
#include "RenderPass.h"
#include "Pipeline.h"

#include "Vertex.h"
#include "Buffer.h"
BufferVertex b;

void Application::Start() {
   mWindow = new Window();
   mWindow->Create(800, 600, "vulkan");
   mVkManager = new VulkanManager();
   mVkManager->Create(mWindow);

   VertexSimple verts[6];
   verts[0].pos = glm::vec2(-1.0f, -1.0f);
   verts[1].pos = glm::vec2(1.0f, -1.0f);
   verts[2].pos = glm::vec2(-1.0f, 1.0f);
   verts[3].pos = glm::vec2(1.0f, -1.0f);
   verts[4].pos = glm::vec2(1.0f, 1.0f);
   verts[5].pos = glm::vec2(-1.0f, 1.0f);
   b.Create(sizeof(VertexSimple)*6);
   BufferStaging staging;
   staging.Create(b.GetSize());
   void* data;
   staging.Map(&data);
   memcpy(data, verts, sizeof(VertexSimple) * 6);
   staging.UnMap();
   //Pre first run setup
   {
      VkCommandBuffer buffer;
      mVkManager->OneTimeCommandBufferStart(buffer);
      {
         VkBufferCopy copyRegion{};
         copyRegion.size = b.GetSize();
         vkCmdCopyBuffer(buffer, staging.GetBuffer(), b.GetBuffer(), 1, &copyRegion);
      }
      mVkManager->OneTimeCommandBufferEnd(buffer);
   }
   staging.Destroy();

   Pipeline test;
   //test.AddShader(GetWorkDir()+"test.frag");
   //test.AddShader(GetWorkDir()+"test.vert");
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
   mVkManager->WaitDevice();
   b.Destroy();
   mVkManager->Destroy();
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}

void Application::Draw() {
   uint32_t frameIndex;
   VkCommandBuffer buffer;
   if (mVkManager->RenderStart(buffer, frameIndex) == false) {
      ASSERT_RET("Failed to start render");
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
   VkBuffer vertexBuffer[] = { b.GetBuffer() };
   VkDeviceSize offsets[] = { 0 };
   vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffer, offsets);
   vkCmdBeginRenderPass(buffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdEndRenderPass(buffer);
   vkEndCommandBuffer(buffer);


   mVkManager->RenderEnd();
}
