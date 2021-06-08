#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "VulkanManager.h"
#include "Window.h"

#include "Framebuffer.h"
#include "RenderPass.h"

#include "Vertex.h"
#include "RenderTarget.h"

#include "Buffer.h"
#include "Image.h"

RenderPass rp;
RenderTarget rt;

#define MAX_DESCRIPTOR_SETS 50
VkDescriptorPoolSize poolSizes[2] = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX_DESCRIPTOR_SETS }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DESCRIPTOR_SETS} };
VkDescriptorPool mDescriptorPool;
VkDescriptorSetLayout mObjectDescriptorSet;
VkDescriptorSet mSceneSet;
VkDescriptorSet mObjectSet;
BufferRingUniform mSceneBuffer;
BufferRingUniform mObjectBuffer;

SceneUBO mSceneUbo{};
ObjectUBO mObjectUbo{};

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
   {
      void* data;
      staging.Map(&data);
      memcpy(data, verts, sizeof(VertexSimple) * 6);
      staging.UnMap();
   }

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

   rp.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_FORMAT_D32_SFLOAT_S8_UINT);
   rt.Create(mVkManager->GetDevice(), &rp, mVkManager->GetSwapchainExtent(), true);

   VkDescriptorSetLayoutBinding sceneLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 0);
   VkDescriptorSetLayoutBinding objectLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1);
   VkDescriptorSetLayoutBinding bindings[] = { sceneLayout, objectLayout };
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = 2;
   layoutInfo.pBindings = bindings;
   vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mObjectDescriptorSet);

   mPipeline.AddShader(GetWorkDir() + "normal.vert");
   mPipeline.AddShader(GetWorkDir() + "normal.frag");
   mPipeline.SetVertexType(VertexTypeDefault);
   mPipeline.AddDescriptorSetLayout(mObjectDescriptorSet);
   mPipeline.Create(mVkManager->GetSwapchainExtent(), &rp);

   mPipelineTest.AddShader(GetWorkDir() + "test.vert");
   mPipelineTest.AddShader(GetWorkDir() + "test.frag");
   mPipelineTest.SetVertexType(VertexTypeSimple);
   mPipelineTest.Create(mVkManager->GetSwapchainExtent(), &rp);

   mModelTest.LoadModel(GetWorkDir()+"Sponza/Sponza.obj");


   VkDescriptorPoolCreateInfo poolCreate{};
   poolCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolCreate.maxSets = MAX_DESCRIPTOR_SETS;
   poolCreate.poolSizeCount = SIZEOF_ARRAY(poolSizes);
   poolCreate.pPoolSizes = poolSizes;
   vkCreateDescriptorPool(mVkManager->GetDevice(), &poolCreate, GetAllocationCallback(), &mDescriptorPool);

   VkDescriptorSetAllocateInfo setAllocate{};
   setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   setAllocate.descriptorPool = mDescriptorPool;
   setAllocate.descriptorSetCount = 1;
   setAllocate.pSetLayouts = &mObjectDescriptorSet;
   vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mSceneSet);


   mSceneBuffer.Create(3, sizeof(SceneUBO), mSceneSet, 0);
   mObjectBuffer.Create(100, sizeof(ObjectUBO), mSceneSet, 1);
  
   Image img;
   img.LoadImage(GetWorkDir() + "Sponza/textures/background.tga");
   img.Destroy();
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

   {
      VkDescriptorSet sets[] = { mSceneSet, mObjectSet };
      //vkFreeDescriptorSets(mVkManager->GetDevice(), mDescriptorPool, 2, sets); //needs VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
      mSceneBuffer.Destroy();
      mObjectBuffer.Destroy();
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mObjectDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorPool(mVkManager->GetDevice(), mDescriptorPool, GetAllocationCallback());
   }

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

   //update
   {
      static glm::vec3 pos = glm::vec3(130, 50, 150);
      ImGui::Begin("Camera");
      ImGui::DragFloat3("Pos", glm::value_ptr(pos));//, 0.01f);
      ImGui::End();
      glm::mat4 proj = glm::perspective(glm::radians(45.0f), mVkManager->GetSwapchainAspect(), 0.1f, 10000.0f);
      proj[1][1] *= -1;
      glm::mat4 view = glm::lookAt(pos, glm::vec3(0, 0, 0), glm::vec3(0, 1.0f, 0));
      mSceneUbo.m_ViewProj = proj * view;

      {
         void* data;
         mSceneBuffer.Get(&data);
         memcpy(data, &mSceneUbo, sizeof(SceneUBO));
         mSceneBuffer.Return();
      }

      mModelTest.SetRotation(glm::vec3(0, frameCounter * 0.07f, 0));
   }

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

   VkClearValue clearColor[2];
   clearColor[0].color.float32[0] = abs(sin((frameCounter * 0.5f) / 5000.0f));
   clearColor[0].color.float32[1] = abs(sin((frameCounter * 0.2f) / 5000.0f));
   clearColor[0].color.float32[2] = abs(sin((frameCounter * 0.1f) / 5000.0f));
   clearColor[0].color.float32[3] = 1.0f;
   clearColor[1].depthStencil.depth = 1.0f;
   clearColor[1].depthStencil.stencil = 0.0f;

   VkRenderPassBeginInfo renderBegin{};
   renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderBegin.renderArea.extent = rt.GetSize();
   renderBegin.renderPass = rp.GetRenderPass();//mVkManager->GetPresentRenderPass()->GetRenderPass();
   renderBegin.clearValueCount = 2;
   renderBegin.pClearValues = clearColor;
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
   //uint32_t descriptorSetOffsets[] = { (frameIndex * sizeof(SceneUBO)),0};
   //vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipelineLayout(), 0, 1, &mSceneSet, 2, descriptorSetOffsets);

   Descriptor des = Descriptor(buffer, mPipeline.GetPipelineLayout(), &mSceneBuffer, &mObjectBuffer, []() {});
   mObjectBuffer.Get();
   mModelTest.Render(&des, RenderMode::NORMAL);
   mObjectBuffer.Return();

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
