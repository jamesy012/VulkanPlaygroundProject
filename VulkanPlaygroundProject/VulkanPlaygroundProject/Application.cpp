#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "VulkanManager.h"
#include "Window.h"

#include "Framebuffer.h"
#include "RenderPass.h"

#include "Vertex.h"
#include "RenderTarget.h"

RenderPass rp;
RenderTarget rt;

void SetImageLayout(VkCommandBuffer aBuffer, VkImage aImage, VkImageAspectFlags aAspectMask, VkImageLayout aOldImageLayout,
                      VkImageLayout aNewImageLayout, VkPipelineStageFlags aSrcStages, VkPipelineStageFlags aDestStages) {

   VkImageMemoryBarrier image_memory_barrier = {};
   image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   image_memory_barrier.pNext = NULL;
   image_memory_barrier.srcAccessMask = 0;
   image_memory_barrier.dstAccessMask = 0;
   image_memory_barrier.oldLayout = aOldImageLayout;
   image_memory_barrier.newLayout = aNewImageLayout;
   image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   image_memory_barrier.image = aImage;
   image_memory_barrier.subresourceRange.aspectMask = aAspectMask;
   image_memory_barrier.subresourceRange.baseMipLevel = 0;
   image_memory_barrier.subresourceRange.levelCount = 1;
   image_memory_barrier.subresourceRange.baseArrayLayer = 0;
   image_memory_barrier.subresourceRange.layerCount = 1;

   switch (aOldImageLayout) {
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
         image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
         image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_PREINITIALIZED:
         image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
         break;

      default:
         break;
   }

   switch (aNewImageLayout) {
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
         break;

      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         break;

      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         break;

      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
         image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
         break;

      default:
         break;
   }

   vkCmdPipelineBarrier(aBuffer, aSrcStages, aDestStages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

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
   mScreenQuad.Create(sizeof(VertexSimple)*6);
   BufferStaging staging;
   staging.Create(mScreenQuad.GetSize());
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
         copyRegion.size = mScreenQuad.GetSize();
         vkCmdCopyBuffer(buffer, staging.GetBuffer(), mScreenQuad.GetBuffer(), 1, &copyRegion);
      }
      mVkManager->OneTimeCommandBufferEnd(buffer);
   }
   staging.Destroy();

   mPipeline.AddShader(GetWorkDir() + "normal.vert");
   mPipeline.AddShader(GetWorkDir() + "normal.frag");
   mPipeline.SetVertexType(VertexTypeDefault);
   mPipeline.Create(mVkManager->GetSwapchainExtent(), mVkManager->GetPresentRenderPass());

   mPipelineTest.AddShader(GetWorkDir() + "test.vert");
   mPipelineTest.AddShader(GetWorkDir() + "test.frag");
   mPipelineTest.SetVertexType(VertexTypeSimple);
   mPipelineTest.Create(mVkManager->GetSwapchainExtent(), mVkManager->GetPresentRenderPass());

   mModelTest.LoadModel(GetWorkDir()+"Sponza/glTF/Sponza.gltf");

   rp.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
   rt.Create(mVkManager->GetDevice(), &rp, mVkManager->GetSwapchainExtent(), false);
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

   rt.Destroy();
   rp.Destroy(mVkManager->GetDevice());
   mScreenQuad.Destroy();
   mPipeline.Destroy();
   mPipelineTest.Destroy();
   mModelTest.Destroy();
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
   vkBeginCommandBuffer(buffer, &beginInfo);

   if (mVkManager->DidResizeLastFrame()) {
      for (int i = 0; i < 3; i++) {
         SetImageLayout(buffer, mVkManager->GetPresentImage(i), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      }
   }

   VkClearValue clearColor{};
   clearColor.color.float32[0] = abs(sin((frameCounter * 0.5f) / 5000.0f));
   clearColor.color.float32[1] = abs(sin((frameCounter * 0.2f) / 5000.0f));
   clearColor.color.float32[2] = abs(sin((frameCounter * 0.1f) / 5000.0f));
   clearColor.color.float32[3] = 1.0f;

   VkRenderPassBeginInfo renderBegin{};
   renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderBegin.renderArea.extent = rt.GetSize();
   renderBegin.renderPass = rp.GetRenderPass();//mVkManager->GetPresentRenderPass()->GetRenderPass();
   renderBegin.clearValueCount = 1;
   renderBegin.pClearValues = &clearColor;
   renderBegin.framebuffer = rt.GetFramebuffer().GetFramebuffer();//mVkManager->GetPresentFramebuffer(frameIndex)->GetFramebuffer();
   vkCmdBeginRenderPass(buffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
   VkViewport viewport = rt.GetViewport();
   VkRect2D scissor = { {}, rt.GetSize() };
   vkCmdSetViewport(buffer, 0, 1, &viewport);
   vkCmdSetScissor(buffer, 0, 1, &scissor);

   vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineTest.GetPipeline());
   VkBuffer vertexBuffer[] = { mScreenQuad.GetBuffer() };
   VkDeviceSize offsets[] = { 0 };
   vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffer, offsets);
   vkCmdDraw(buffer, 6, 1, 0, 0);

   vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipeline());
   mModelTest.Render(buffer, mPipeline.GetPipelineLayout(), RenderMode::NORMAL);

   vkCmdEndRenderPass(buffer);

   //VkImageMemoryBarrier barrier{};
   //barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   //barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   //barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   //barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   //barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   //barrier.image = mVkManager->GetPresentImage(frameIndex);
   //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   //barrier.subresourceRange.baseMipLevel = 0;
   //barrier.subresourceRange.levelCount = 1;
   //barrier.subresourceRange.baseArrayLayer = 0;
   //barrier.subresourceRange.layerCount = 1;
   //
   //vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
   //
   SetImageLayout(buffer, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   VkImageBlit blit{};
   blit.srcOffsets[1].x = rt.GetSize().width;
   blit.srcOffsets[1].y = rt.GetSize().height;
   blit.srcOffsets[1].z = 1;
   blit.dstOffsets[1].x = mVkManager->GetSwapchainExtent().width;
   blit.dstOffsets[1].y = mVkManager->GetSwapchainExtent().height;
   blit.dstOffsets[1].z = 1;
   blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.srcSubresource.layerCount = 1;
   blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.dstSubresource.layerCount = 1;
   vkCmdBlitImage(buffer, rt.GetColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);
   SetImageLayout(buffer, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

   //barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   //barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   //vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);

   //renderBegin = {};
   //renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   //renderBegin.renderArea.extent = mVkManager->GetSwapchainExtent();
   //renderBegin.renderPass = mVkManager->GetPresentRenderPass()->GetRenderPass();
   //renderBegin.clearValueCount = 1;
   //renderBegin.pClearValues = &clearColor;
   //renderBegin.framebuffer = mVkManager->GetPresentFramebuffer(frameIndex)->GetFramebuffer();
   //vkCmdBeginRenderPass(buffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
   //vkCmdEndRenderPass(buffer);
   vkEndCommandBuffer(buffer);

   mVkManager->RenderEnd();
}
