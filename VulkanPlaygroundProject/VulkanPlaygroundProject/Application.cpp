#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "VulkanManager.h"
#include "Window.h"
#include "InputHandler.h"

#include "Vertex.h"

#include "Shadow.h"

ShadowDirectional mShadow;
ShadowDirectionalCascade mShadowCascade;

void Application::Start() {
   mWindow = new Window();
   mWindow->Create(800, 600, "vulkan");
   mVkManager = new VulkanManager();
   mVkManager->Create(mWindow);
   _CInput = new InputHandler();
   _CInput->Startup(mWindow->GetWindow());

   ShadowManager::Create();

   mScreenQuad.CreatePrimitive(VertexPrimitives::QUAD);
   mBillboardQuad.CreatePrimitive(VertexPrimitives::QUAD);

   CreateSizeDependent();
   mVkManager->mSizeDependentCreateCallback = std::bind(&Application::CreateSizeDependent, this);
   mVkManager->mSizeDependentDestroyCallback = std::bind(&Application::DestroySizeDependent, this);

   mShadow.Create({ 2048, 2048 }, VK_NULL_HANDLE);
   mShadowCascade.Create({ 2048, 2048 }, VK_NULL_HANDLE);

   {
      {
         VkDescriptorSetLayoutBinding sceneLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
         VkDescriptorSetLayoutBinding bindings[] = { sceneLayout };
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = 1;
         layoutInfo.pBindings = bindings;
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mSceneDescriptorSet);
      }
      {
         VkDescriptorSetLayoutBinding objectLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 0);
         VkDescriptorSetLayoutBinding bindings[] = { objectLayout };
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = 1;
         layoutInfo.pBindings = bindings;
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mObjectDescriptorSet);
      }
      {
         VkDescriptorSetLayoutBinding diffuseLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
         VkDescriptorSetLayoutBinding shadowLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
         std::vector<VkDescriptorSetLayoutBinding> bindings = { diffuseLayout, shadowLayout };
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
         layoutInfo.pBindings = bindings.data();
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mMaterialDescriptorSet);
      }

      {
         mPipeline.AddShader(GetWorkDir() + "normal.vert");
         mPipeline.AddShader(GetWorkDir() + "normal.frag");
         mPipeline.SetVertexType(VertexTypeDefault);
         mPipeline.AddDescriptorSetLayout(mSceneDescriptorSet);
         mPipeline.AddDescriptorSetLayout(mObjectDescriptorSet);
         mPipeline.AddDescriptorSetLayout(mMaterialDescriptorSet);
         mPipeline.SetBlendingEnabled(true);
         mPipeline.Create(mVkManager->GetSwapchainExtent(), &mRenderPass);

         mPipelineTest.AddShader(GetWorkDir() + "test.vert");
         mPipelineTest.AddShader(GetWorkDir() + "test.frag");
         mPipelineTest.SetVertexType(VertexTypeSimple);
         mPipelineTest.Create(mVkManager->GetSwapchainExtent(), &mRenderPass);

         mPipelineShadow.AddShader(GetWorkDir() + "normal.vert");
         //mPipelineShadow.AddShader(GetWorkDir() + "test.frag");
         mPipelineShadow.SetVertexType(VertexTypeDefault);
         mPipelineShadow.AddDescriptorSetLayout(mSceneDescriptorSet);
         mPipelineShadow.AddDescriptorSetLayout(mObjectDescriptorSet);
         //mPipelineShadow.SetCullMode(VK_CULL_MODE_FRONT_BIT);
         mPipelineShadow.Create(mVkManager->GetSwapchainExtent(), ShadowManager::GetRenderPass());
      }

      {
         VkDescriptorSetAllocateInfo setAllocate{};
         setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
         setAllocate.descriptorPool = mVkManager->GetDescriptorPool();
         setAllocate.descriptorSetCount = 1;
         setAllocate.pSetLayouts = &mSceneDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mSceneSet);
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mSceneShadowSet);

         setAllocate.pSetLayouts = &mObjectDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mObjectSet);

         setAllocate.pSetLayouts = &mMaterialDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mMaterialSet);


         mSceneBuffer.Create(3, sizeof(SceneUBO), mSceneSet, 0);
         mSceneBuffer.SetName("Scene Buffer");
         mSceneShadowBuffer.Create(3, sizeof(SceneUBO), mSceneShadowSet, 0);
         mSceneShadowBuffer.SetName("Scene Shadow Buffer");
         mObjectBuffer.Create(500, sizeof(ObjectUBO), mObjectSet, 0);
         mObjectBuffer.SetName("Object Buffer");
      }
   }

   mTestImg.LoadImage(GetWorkDir() + "Sponza/textures/background.tga");
   UpdateImageDescriptorSet(&mTestImg, mMaterialSet, mVkManager->GetDefaultSampler(), 0);
   std::vector<VkDescriptorImageInfo> imgInfo = std::vector<VkDescriptorImageInfo>(1);
   std::vector<VkWriteDescriptorSet> writeSets = { GetWriteDescriptorSet(imgInfo[0], mShadow.GetImage(), mMaterialSet, mVkManager->GetDefaultClampedSampler(), 1) };
   vkUpdateDescriptorSets(_VulkanManager->GetDevice(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
   UpdateImageDescriptorSet(mShadow.GetImage(), mMaterialSet, mVkManager->GetDefaultClampedSampler(), 1);

   mModelTest.LoadModel(GetWorkDir() + "Sponza/Sponza.obj", mMaterialDescriptorSet, writeSets);
   mModelTest.SetScale(0.05f);

   //mFlyCamera.SetPosition(glm::vec3(130, 50, 150));
   mFlyCamera.SetFarClip(200.0f);
   mLightPos = glm::vec3(-20, 74, 10);
}

static bool sStartProfile = false;

void Application::Run() {
   while (!mWindow->ShouldClose()) {
      if (sStartProfile) {
         Profiler::Get().StartProfile();
         sStartProfile = false;
      }
      PROFILE_START_SCOPED("Frame");
      mWindow->Update();
      mVkManager->Update();

      _CInput->Update();

      Update();

      ImGui();

      Draw();
   }
}

static uint32_t frameCounter = 0;

void Application::ImGui() {
   PROFILE_START_SCOPED("ImGui - application update");
   ImGuiIO& io = ImGui::GetIO();
   _CInput->AllowInput(!io.WantCaptureMouse && mWindow->IsFocused() && mWindow->IsHovered());


   ImGui::Begin("stats");
   ImGui::Text("size: %iX%i", mRenderTarget.GetSize().width, mRenderTarget.GetSize().height);
   ImGui::Text("Window Size: %iX%i", mVkManager->GetSwapchainExtent().width, mVkManager->GetSwapchainExtent().height);
   ImGui::Text("FPS: %f(%f)", io.Framerate, io.DeltaTime);
#if ALLOW_PROFILING
   ImGui::Text("Profiling:");
   ImGui::SameLine();
   if (ImGui::Button("Begin")) {
      sStartProfile = true;
   }
   ImGui::SameLine();
   if (ImGui::Button("End")) {
      Profiler::Get().EndProfile();
   }
#endif
   ImGui::End();

   ImGui::Begin("scene");
   ImGui::DragFloat3("LightPos", glm::value_ptr(mLightPos), 0.05f);
   ImGui::End();
}

void Application::Update() {
   PROFILE_START_SCOPED("Update");

   mFlyCamera.UpdateInput();
   mSceneUbo.mViewProj = mFlyCamera.GetPV();
   mSceneUbo.mViewPos = glm::vec4(mFlyCamera.GetPostion(), 0);
   mSceneUbo.mLightPos = glm::vec4(mLightPos, 0);

   glm::mat4 shadowProj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 200.0f);
   glm::mat4 shadowView = glm::lookAt(mLightPos, glm::vec3(0), glm::vec3(0, 1, 0));
   mSceneUbo.mLightProj = shadowProj * shadowView;

   {
      void* data;
      mSceneBuffer.Get(&data);
      memcpy(data, &mSceneUbo, sizeof(SceneUBO));
      mSceneBuffer.Return();
   }

   mSceneShadowUbo.mViewProj = mSceneUbo.mLightProj;
   mSceneShadowUbo.mLightProj = mSceneUbo.mLightProj;
   mSceneShadowUbo.mViewPos = glm::vec4(mLightPos, 0);
   mSceneShadowUbo.mLightPos = glm::vec4(mLightPos, 0);

   {
      void* data;
      mSceneShadowBuffer.Get(&data);
      memcpy(data, &mSceneShadowUbo, sizeof(SceneUBO));
      mSceneShadowBuffer.Return();
   }

   //mModelTest.SetRotation(glm::vec3(0, frameCounter * 0.07f, 0));
}

void Application::CreateSizeDependent() {
   if (!mRenderPass.IsValid()) {
      mRenderPass.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_FORMAT_D32_SFLOAT_S8_UINT);
   }
   mRenderTarget.Create(mVkManager->GetDevice(), &mRenderPass, mVkManager->GetSwapchainExtent(), true);
}

void Application::DestroySizeDependent() {
   mRenderTarget.Destroy();
}


void Application::Draw() {
   PROFILE_START_SCOPED("Draw");
   uint32_t frameIndex;
   VkCommandBuffer buffer;
   if (mVkManager->RenderStart(buffer, frameIndex) == false) {
      ASSERT_RET("Failed to start render");
   }
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

   //shadow
   {
      mShadow.StartRenderPass(buffer);

      vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineShadow.GetPipeline());
      uint32_t descriptorSetOffsets[] = { mSceneShadowBuffer.GetCurrentOffset() };
      vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineShadow.GetPipelineLayout(), 0, 1, &mSceneShadowSet, 1, descriptorSetOffsets);
      {
         DescriptorUBO des = DescriptorUBO(buffer, mPipelineShadow.GetPipelineLayout(), &mObjectBuffer);
         mModelTest.Render(&des, RenderMode::SHADOW);
      }

      mShadow.EndRenderPass(buffer);
   }
   //cascade shadow
   {
      for (uint32_t i = 0; i < mShadowCascade.NumCascades(); i++) {
         mShadowCascade.StartRenderPass(buffer, i);

         vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineShadow.GetPipeline());
         uint32_t descriptorSetOffsets[] = { mSceneShadowBuffer.GetCurrentOffset() };
         vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineShadow.GetPipelineLayout(), 0, 1, &mSceneShadowSet, 1, descriptorSetOffsets);
         {
            DescriptorUBO des = DescriptorUBO(buffer, mPipelineShadow.GetPipelineLayout(), &mObjectBuffer);
            mModelTest.Render(&des, RenderMode::SHADOW);
         }

         mShadowCascade.EndRenderPass(buffer, i);
      }
   }
   //main render
   {
      _VulkanManager->DebugMarkerStart(buffer, "Main Render", glm::vec4(0.0f, 0.3f, 0.0f, 0.2f));
      std::vector<VkClearValue> clearColor = std::vector<VkClearValue>(2);
      clearColor[0].color.float32[0] = abs(sin((frameCounter * 0.5f) / 5000.0f));
      clearColor[0].color.float32[1] = abs(sin((frameCounter * 0.2f) / 5000.0f));
      clearColor[0].color.float32[2] = abs(sin((frameCounter * 0.1f) / 5000.0f));
      clearColor[0].color.float32[3] = 1.0f;
      clearColor[1].depthStencil.depth = 1.0f;
      clearColor[1].depthStencil.stencil = 0;
      mRenderTarget.StartRenderPass(buffer, clearColor);

      vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineTest.GetPipeline());

      uint32_t descriptorSetOffsets[] = { mSceneBuffer.GetCurrentOffset() };
      vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipelineLayout(), 0, 1, &mSceneSet, 1, descriptorSetOffsets);

      mScreenQuad.Bind(buffer);
      vkCmdDraw(buffer, 6, 1, 0, 0);

      vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipeline());
      vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipelineLayout(), 2, 1, &mMaterialSet, 0, nullptr);

      {
         DescriptorUBO des = DescriptorUBO(buffer, mPipeline.GetPipelineLayout(), &mObjectBuffer);
         mModelTest.Render(&des, RenderMode::NORMAL);
         {
            ObjectUBO ubo;
            ubo.mModel = glm::translate(glm::identity<glm::mat4>(), mLightPos);
            ubo.mModel = ubo.mModel * glm::mat4(mFlyCamera.GetRotationQuat());
            ubo.mModel = glm::scale(ubo.mModel, glm::vec3(0.5f));
            des.UpdateObjectAndBind(&ubo);

            mBillboardQuad.Bind(buffer);
            vkCmdDraw(buffer, 6, 1, 0, 0);
         }
      }

      mRenderTarget.EndRenderPass(buffer);
      _VulkanManager->DebugMarkerEnd(buffer);
   }

   _VulkanManager->DebugMarkerStart(buffer, "Present Prepare");
   SetImageLayout(buffer, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   VkImageBlit blit{};
   blit.srcOffsets[1].x = mRenderTarget.GetSize().width;
   blit.srcOffsets[1].y = mRenderTarget.GetSize().height;
   blit.srcOffsets[1].z = 1;
   blit.dstOffsets[1].x = mVkManager->GetSwapchainExtent().width;
   blit.dstOffsets[1].y = mVkManager->GetSwapchainExtent().height;
   blit.dstOffsets[1].z = 1;
   blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.srcSubresource.layerCount = 1;
   blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.dstSubresource.layerCount = 1;
   vkCmdBlitImage(buffer, mRenderTarget.GetColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);
   SetImageLayout(buffer, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
   _VulkanManager->DebugMarkerEnd(buffer);

   vkEndCommandBuffer(buffer);

   mVkManager->RenderEnd();
}


void Application::Destroy() {
   _CInput->Shutdown();
   delete _CInput;
   mVkManager->WaitDevice();

   mTestImg.Destroy();
   mShadowCascade.Destroy();
   mShadow.Destroy();

   {
      VkDescriptorSet sets[] = { mSceneSet, mObjectSet, mMaterialSet };
      //vkFreeDescriptorSets(mVkManager->GetDevice(), mDescriptorPool, 2, sets); //needs VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
      mSceneShadowBuffer.Destroy();
      mSceneBuffer.Destroy();
      mObjectBuffer.Destroy();
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mSceneDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mObjectDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mMaterialDescriptorSet, GetAllocationCallback());
   }

   mRenderTarget.Destroy();
   mRenderPass.Destroy(mVkManager->GetDevice());
   mBillboardQuad.Destroy();
   mScreenQuad.Destroy();
   mPipelineShadow.Destroy();
   mPipeline.Destroy();
   mPipelineTest.Destroy();
   mModelTest.Destroy();

   ShadowManager::Destroy();

   mVkManager->Destroy();
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}
