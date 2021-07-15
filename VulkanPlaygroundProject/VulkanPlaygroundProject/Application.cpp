#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "VulkanManager.h"
#include "Window.h"
#include "InputHandler.h"

#include "Vertex.h"

#include "Shadow.h"

#include "Terrain.h"

ShadowDirectional mShadowCascade;

uint32_t shadowOffsets[NUM_SHADOW_CASCADES];
float shadowOffsetsSplitDepth[NUM_SHADOW_CASCADES];
glm::mat4 shadowOffsetsVpMatrix[NUM_SHADOW_CASCADES];
float cascadeSplitLambda = 0.8f;

Pipeline mTerrainTestPipeline;
Pipeline mTerrainTestShadowPipeline;
Terrain mTerrainTest;
VkDescriptorSetLayout mTerrainTestHeightMapSetLayout;
VkDescriptorSet mTerrainTestHeightMapSet;
Image mTerrainHeightMapImage;
Image mTerrainAlbedoImages;

RenderTarget mScreenSpaceRT;
VkDescriptorSetLayout mScreenSpaceSetLayout;
VkDescriptorSet mScreenSpaceSet;
Pipeline mTerrainSSGPipeline;

struct SSG {
   int enabled = true;
   float timer = 0.0f;
} mSSGPushConstants;


struct ComputeTestStruct {
   glm::mat4 matrices[64];
};

void Application::Start() {
   mWindow = new Window();
   mWindow->Create(800, 600, "vulkan");
   mVkManager = new VulkanManager();
   mVkManager->Create(mWindow);
   _CInput = new InputHandler();
   _CInput->Startup(mWindow->GetWindow());

   ShadowManager::Create();

   {
      bool result = mScreenQuad.Create(sizeof(VertexSimple) * 6);
      VertexSimple verts[6]{};
      verts[0].pos = glm::vec2(-1.0f, 1.0f);
      verts[1].pos = glm::vec2(1.0f, 1.0f);
      verts[2].pos = glm::vec2(1.0f, -1.0f);
      verts[3].pos = glm::vec2(1.0f, -1.0f);
      verts[4].pos = glm::vec2(-1.0f, -1.0f);
      verts[5].pos = glm::vec2(-1.0f, 1.0f);
      BufferStaging staging;
      staging.Create(sizeof(VertexSimple) * 6);
      {
         void* data;
         staging.Map(&data);
         memcpy(data, verts, sizeof(VertexSimple) * 6);
         staging.UnMap();
      }
      mScreenQuad.CopyFrom(&staging);
      staging.Destroy();
   }
   mBillboardQuad.CreatePrimitive(VertexPrimitives::QUAD);

   CreateSizeDependent();
   mVkManager->mSizeDependentCreateCallback = std::bind(&Application::CreateSizeDependent, this);
   mVkManager->mSizeDependentDestroyCallback = std::bind(&Application::DestroySizeDependent, this);

   mShadowCascade.Create({ 2048, 2048 }, VK_NULL_HANDLE, NUM_SHADOW_CASCADES);

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
         VkDescriptorSetLayoutBinding diffuseLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
         VkDescriptorSetLayoutBinding depthLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
         std::vector<VkDescriptorSetLayoutBinding> bindings = { diffuseLayout, depthLayout };
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
         layoutInfo.pBindings = bindings.data();
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mScreenSpaceSetLayout);
      }
      {
         VkDescriptorSetLayoutBinding heightmapLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT, 0);
         VkDescriptorSetLayoutBinding shadowLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
         VkDescriptorSetLayoutBinding diffuseLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
         std::vector<VkDescriptorSetLayoutBinding> bindings = { heightmapLayout, shadowLayout, diffuseLayout};
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
         layoutInfo.pBindings = bindings.data();
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mTerrainTestHeightMapSetLayout);
      }

      {
         VkDescriptorSetLayoutBinding sceneLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_COMPUTE_BIT, 0);
         VkDescriptorSetLayoutBinding inputLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
         VkDescriptorSetLayoutBinding outputLayout = CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2);
         VkDescriptorSetLayoutBinding bindings[] = { sceneLayout, inputLayout, outputLayout };
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
         layoutInfo.pBindings = bindings;
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mComputeTestDescriptorSet);
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

         mTerrainTestPipeline.AddShader(GetWorkDir() + "terrain.vert");
         mTerrainTestPipeline.AddShader(GetWorkDir() + "terrain.frag");
         ShaderMacroArguments terrainTestArguments;
         terrainTestArguments.mMacros = { ShaderMacroArguments::Args::SIMPLE_SCENE };
         //mTerrainTestPipeline.SetShaderMacroArguments(terrainTestArguments);
         mTerrainTestPipeline.SetVertexType(VertexTypeDefault);
         mTerrainTestPipeline.AddDescriptorSetLayout(mSceneDescriptorSet);
         mTerrainTestPipeline.AddDescriptorSetLayout(mObjectDescriptorSet);
         mTerrainTestPipeline.AddDescriptorSetLayout(mTerrainTestHeightMapSetLayout);
         //mTerrainTestPipeline.SetBlendingEnabled(true);
         mTerrainTestPipeline.Create(mVkManager->GetSwapchainExtent(), &mRenderPass);

         mTerrainTestShadowPipeline.AddShader(GetWorkDir() + "terrain.vert");
         mTerrainTestShadowPipeline.SetVertexType(VertexTypeDefault);
         mTerrainTestShadowPipeline.AddDescriptorSetLayout(mSceneDescriptorSet);
         mTerrainTestShadowPipeline.AddDescriptorSetLayout(mObjectDescriptorSet);
         mTerrainTestShadowPipeline.AddDescriptorSetLayout(mTerrainTestHeightMapSetLayout);
         mTerrainTestShadowPipeline.SetCullMode(VK_CULL_MODE_FRONT_BIT);
         mTerrainTestShadowPipeline.Create(mVkManager->GetSwapchainExtent(), ShadowManager::GetRenderPass());

         mPipelineTest.AddShader(GetWorkDir() + "test.vert");
         mPipelineTest.AddShader(GetWorkDir() + "test.frag");
         mPipelineTest.SetVertexType(VertexTypeSimple);
         mPipelineTest.Create(mVkManager->GetSwapchainExtent(), &mRenderPass);

         ShaderMacroArguments shadowArguments;
         shadowArguments.mMacros = {ShaderMacroArguments::Args::POSITION_ONLY, ShaderMacroArguments::Args::SIMPLE_SCENE};
         mPipelineShadow.SetShaderMacroArguments(shadowArguments);
         mPipelineShadow.AddShader(GetWorkDir() + "normal.vert");
         mPipelineShadow.SetVertexType(VertexTypeDefault);
         mPipelineShadow.AddDescriptorSetLayout(mSceneDescriptorSet);
         mPipelineShadow.AddDescriptorSetLayout(mObjectDescriptorSet);
         //mPipelineShadow.SetCullMode(VK_CULL_MODE_FRONT_BIT);
         mPipelineShadow.Create(mVkManager->GetSwapchainExtent(), ShadowManager::GetRenderPass());

         mComputeTest.AddShader(GetWorkDir() + "computeTest.comp");
         mComputeTest.AddDescriptorSetLayout(mComputeTestDescriptorSet);
         mComputeTest.Create(mVkManager->GetSwapchainExtent(), nullptr);

         mTerrainSSGPipeline.AddShader(GetWorkDir() + "test.vert");
         mTerrainSSGPipeline.AddShader(GetWorkDir() + "SSG.frag");
         mTerrainSSGPipeline.SetVertexType(VertexTypeSimple);
         mTerrainSSGPipeline.AddDescriptorSetLayout(mScreenSpaceSetLayout);
         mTerrainSSGPipeline.AddPushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSG));
         mTerrainSSGPipeline.Create(mVkManager->GetSwapchainExtent(), &mRenderPassNoDepth);
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
         setAllocate.pSetLayouts = &mTerrainTestHeightMapSetLayout;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mTerrainTestHeightMapSet); 

         setAllocate.pSetLayouts = &mScreenSpaceSetLayout;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mScreenSpaceSet);

         setAllocate.pSetLayouts = &mComputeTestDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mComputeTestSet);


         mSceneBuffer.Create(3, sizeof(SceneUBO), mSceneSet, 0);
         mSceneBuffer.SetName("Scene Buffer");
         mSceneShadowBuffer.Create(mShadowCascade.NumCascades(), sizeof(SceneSimpleUBO), mSceneShadowSet, 0);
         mSceneShadowBuffer.SetName("Scene Shadow Buffer");
         mObjectBuffer.Create(500, sizeof(ObjectUBO), mObjectSet, 0);
         mObjectBuffer.SetName("Object Buffer");

         mSceneBuffer.AddToDescriptorSet(mComputeTestSet, 0, true);
         mComputeTestInputBuffer.Create(1, sizeof(ComputeTestStruct), mComputeTestSet, 1, false);
         mComputeTestOutputBuffer.Create(1, sizeof(ComputeTestStruct), mComputeTestSet, 2, false);

      }
   }

   mTestImg.LoadImage(GetWorkDir() + "Sponza/textures/background.tga");
   UpdateImageDescriptorSet(&mTestImg, mMaterialSet, mVkManager->GetDefaultSampler(), 0);
   std::vector<VkDescriptorImageInfo> imgInfo = std::vector<VkDescriptorImageInfo>(1);
   std::vector<VkWriteDescriptorSet> writeSets = { GetWriteDescriptorSet(imgInfo[0], mShadowCascade.GetImage(), mMaterialSet, mVkManager->GetDefaultClampedSampler(), 1) };
   vkUpdateDescriptorSets(_VulkanManager->GetDevice(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
   UpdateImageDescriptorSet(mShadowCascade.GetImage(), mMaterialSet, mVkManager->GetDefaultClampedSampler(), 1);

   mModelTest.LoadModel(GetWorkDir() + "Sponza/Sponza.obj", mMaterialDescriptorSet, writeSets);
   mModelTest.SetScale(0.05f);


   {
      ComputeTestStruct* data;
      mComputeTestInputBuffer.Map(reinterpret_cast<void**>(&data));

      for (int i = 0; i < 64; i++) {
         for (int q = 0; q < 16; q++) {
            glm::value_ptr(data[0].matrices[i])[q] = (float)i + q;
         }
      }

      size_t index = 0;
      for (size_t i = 0; i < mModelTest.GetNumNodes(); i++) {
         Model::Node* node = mModelTest.GetNode(i);
         if (!node->mMesh.empty()) {
            data[0].matrices[index++] = node->GetMatrixWithParents();
         }
      }
         
      mComputeTestInputBuffer.UnMap();
   }

   //mFlyCamera.SetPosition(glm::vec3(130, 50, 150));
   mFlyCamera.SetFarClip(150.0f);
   mLightPos = glm::vec3(-20, 74, 10);

   mTerrainAlbedoImages.CreateImage({ 1024, 1024 }, 3, 1);
   mTerrainAlbedoImages.LoadImageForArray(GetWorkDir() + "Textures/seamlessTextures/100_1382_seamless.JPG", 0);
   mTerrainAlbedoImages.LoadImageForArray(GetWorkDir() + "Textures/seamlessTextures/100_1395_seamless.JPG", 1);
   mTerrainAlbedoImages.LoadImageForArray(GetWorkDir() + "Textures/seamlessTextures/100_1377_seamless.JPG", 2);
   mTerrainHeightMapImage.LoadImage(GetWorkDir() + "Textures/heightmapTest.png");
   UpdateImageDescriptorSet(&mTerrainHeightMapImage, mTerrainTestHeightMapSet, mVkManager->GetDefaultMirrorSampler(), 0);
   UpdateImageDescriptorSet(mShadowCascade.GetImage(), mTerrainTestHeightMapSet, mVkManager->GetDefaultClampedSampler(), 1);
   UpdateImageDescriptorSet(&mTerrainAlbedoImages, mTerrainTestHeightMapSet, mVkManager->GetDefaultSampler(), 2);
   mTerrainTest.Create("");

   CreateDelayedSizeDependent();
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
   ImGui::DragFloat("split", &cascadeSplitLambda, 0.01f);
   ImGui::Checkbox("SSG", (bool*)&mSSGPushConstants.enabled);
   ImGui::End();
}

void Application::Update() {
   PROFILE_START_SCOPED("Update");

   mFlyCamera.UpdateInput();

   //Cascade shadows
   {
      float cascadeSplits[NUM_SHADOW_CASCADES];

      float nearClip = mFlyCamera.GetNear();
      float farClip = mFlyCamera.GetFar();
      float clipRange = farClip - nearClip;

      float minZ = nearClip;
      float maxZ = nearClip + clipRange;

      float range = maxZ - minZ;
      float ratio = maxZ / minZ;

      for (uint32_t i = 0; i < mShadowCascade.NumCascades(); i++) {
         float p = (i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);
         float log = minZ * std::pow(ratio, p);
         float uniform = minZ + range * p;
         float d = cascadeSplitLambda * (log - uniform) + uniform;
         cascadeSplits[i] = (d - nearClip) / clipRange;
      }

      // Calculate orthographic projection matrix for each cascade
      float lastSplitDist = 0.0;
      for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
         float splitDist = cascadeSplits[i];

         glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
         };

         // Project frustum corners into world space
         glm::mat4 invCam = glm::inverse(mFlyCamera.GetPV());
         for (uint32_t i = 0; i < 8; i++) {
            glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
            frustumCorners[i] = invCorner / invCorner.w;
         }

         for (uint32_t i = 0; i < 4; i++) {
            glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
            frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
            frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
         }

         // Get frustum center
         glm::vec3 frustumCenter = glm::vec3(0.0f);
         for (uint32_t i = 0; i < 8; i++) {
            frustumCenter += frustumCorners[i];
         }
         frustumCenter /= 8.0f;

         float radius = 0.0f;
         for (uint32_t i = 0; i < 8; i++) {
            float distance = glm::length(frustumCorners[i] - frustumCenter);
            radius = glm::max(radius, distance);
         }
         radius = std::ceil(radius * 16.0f) / 16.0f;

         glm::vec3 maxExtents = glm::vec3(radius);
         glm::vec3 minExtents = -maxExtents;

         glm::vec3 lightDir = glm::normalize(-mLightPos);
         glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
         glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -75.0f, maxExtents.z - minExtents.z);

         // Store split distance and matrix in cascade
         shadowOffsetsSplitDepth[i] = (mFlyCamera.GetNear() + splitDist * clipRange) * -1.0f;
         shadowOffsetsVpMatrix[i] = lightOrthoMatrix * lightViewMatrix;

         lastSplitDist = cascadeSplits[i];
      }
   }

   //ubo
   {
      //scene
      {
         mSceneUbo.mViewProj = mFlyCamera.GetPV();
         mSceneUbo.mViewPos = glm::vec4(mFlyCamera.GetPostion(), 0);
         mSceneUbo.mLightPos = glm::vec4(mLightPos, 0);

         for (int i = 0; i < NUM_SHADOW_CASCADES; i++) {
            mSceneUbo.mShadowCascadeProj[i] = shadowOffsetsVpMatrix[i];
            glm::value_ptr(mSceneUbo.mShadowSplits)[i] = shadowOffsetsSplitDepth[i];
         }

         {
            void* data;
            mSceneBuffer.Get(&data);
            memcpy(data, &mSceneUbo, sizeof(SceneUBO));
            mSceneBuffer.Return();
         }
      }
      //shadow
      {
         for (uint32_t i = 0; i < mShadowCascade.NumCascades(); i++) {
            mSceneShadowUbo.mViewProj = shadowOffsetsVpMatrix[i];
            void* data;
            mSceneShadowBuffer.Get(&data, shadowOffsets[i]);
            memcpy(data, &mSceneShadowUbo, sizeof(SceneUBO));
            mSceneShadowBuffer.Return();
         }
      }
   }

   {
      mSSGPushConstants.timer += ImGui::GetIO().DeltaTime;
   }
   //mModelTest.SetRotation(glm::vec3(0, frameCounter * 0.07f, 0));
}

void Application::CreateSizeDependent() {
   if (!mRenderPass.IsValid()) {
      mRenderPass.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_FORMAT_D32_SFLOAT_S8_UINT);
   }
   if (!mRenderPassNoDepth.IsValid()) {
      mRenderPassNoDepth.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
   }
   mRenderTarget.Create(mVkManager->GetDevice(), &mRenderPass, mVkManager->GetSwapchainExtent(), true);
   mScreenSpaceRT.Create(mVkManager->GetDevice(), &mRenderPassNoDepth, mVkManager->GetSwapchainExtent(), false);

   if (frameCounter != 0) {
      CreateDelayedSizeDependent();
   }
}

void Application::CreateDelayedSizeDependent() {
   UpdateImageDescriptorSet(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mRenderTarget.GetColorImageView(), mScreenSpaceSet, mVkManager->GetDefaultMirrorSampler(), 0);
   UpdateImageDescriptorSet(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, mRenderTarget.GetDepthImageView(), mScreenSpaceSet, mVkManager->GetDefaultMirrorSampler(), 1);
}

void Application::DestroySizeDependent() {
   mRenderTarget.Destroy();
   mScreenSpaceRT.Destroy();
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

   //cascade shadow
   {
      for (uint32_t i = 0; i < mShadowCascade.NumCascades(); i++) {
         mShadowCascade.StartRenderPass(buffer, i);

         //vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineShadow.GetPipeline());
         //vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineShadow.GetPipelineLayout(), 0, 1, &mSceneShadowSet, 1, &shadowOffsets[i]);
         //{
         //   DescriptorUBO des = DescriptorUBO(buffer, mPipelineShadow.GetPipelineLayout(), &mObjectBuffer, mObjectSet);
         //   mModelTest.Render(&des, RenderMode::SHADOW);
         //}

         //terrain test
         {
            vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainTestShadowPipeline.GetPipeline());
            vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainTestShadowPipeline.GetPipelineLayout(), 0, 1, &mSceneShadowSet, 1, &shadowOffsets[i]);
            vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainTestShadowPipeline.GetPipelineLayout(), 2, 1, &mTerrainTestHeightMapSet, 0, nullptr);
            mTerrainTest.Render(buffer, mTerrainTestShadowPipeline.GetPipelineLayout(), &mObjectBuffer, mObjectSet);
         }

         mShadowCascade.EndRenderPass(buffer, i);
      }
   }
   //main render
   {
      _VulkanManager->DebugMarkerStart(buffer, "Main Render", glm::vec4(0.0f, 0.3f, 0.0f, 0.2f));
      //full screen quad test
      {
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
         //vkCmdDraw(buffer, 6, 1, 0, 0);
      }

      //terrain test
      {
         vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainTestPipeline.GetPipeline());
         vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainTestPipeline.GetPipelineLayout(), 2, 1, &mTerrainTestHeightMapSet, 0, nullptr);

         mTerrainTest.Render(buffer, mTerrainTestPipeline.GetPipelineLayout(), &mObjectBuffer, mObjectSet);
      }

      vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipeline());
      vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipelineLayout(), 2, 1, &mMaterialSet, 0, nullptr);

      //render Light facing camera
      {
         DescriptorUBO des = DescriptorUBO(buffer, mPipeline.GetPipelineLayout(), &mObjectBuffer, mObjectSet);
         //mModelTest.Render(&des, RenderMode::NORMAL);

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

   //compute Test
   {
      _VulkanManager->DebugMarkerStart(buffer, "Compute Test", glm::vec4(0.0f, 0.0f, 0.3f, 0.2f));
      vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputeTest.GetPipeline());

      uint32_t descriptorSetOffsets[] = { mSceneBuffer.GetCurrentOffset() };
      vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputeTest.GetPipelineLayout(), 0, 1, &mComputeTestSet, 1, descriptorSetOffsets);

      vkCmdDispatch(buffer, 1, 1, 1);

      _VulkanManager->DebugMarkerEnd(buffer);

   }

   //screenspace effects
   {
      _VulkanManager->DebugMarkerStart(buffer, "screen space", glm::vec4(0.0f, 0.5f, 0.3f, 0.2f));
      SetImageLayout(buffer, mRenderTarget.GetColorImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
      SetImageLayout(buffer, mRenderTarget.GetDepthImage(), VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
      //grass
      {
         _VulkanManager->DebugMarkerStart(buffer, "grass", glm::vec4(0.5f, 0.0f, 0.3f, 0.3f));
         VkClearValue clearColor;
         clearColor.color.float32[0] = abs(sin((frameCounter * 0.5f) / 5000.0f));
         clearColor.color.float32[1] = abs(sin((frameCounter * 0.2f) / 5000.0f));
         clearColor.color.float32[2] = abs(sin((frameCounter * 0.1f) / 5000.0f));
         clearColor.color.float32[3] = 1.0f;
         mScreenSpaceRT.StartRenderPass(buffer, { clearColor });

         vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainSSGPipeline.GetPipeline());
         vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTerrainSSGPipeline.GetPipelineLayout(), 0, 1, &mScreenSpaceSet, 0, nullptr);
         vkCmdPushConstants(buffer, mTerrainSSGPipeline.GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSG), &mSSGPushConstants);
         mScreenQuad.Bind(buffer);
         vkCmdDraw(buffer, 6, 1, 0, 0);

         mScreenSpaceRT.EndRenderPass(buffer);
         _VulkanManager->DebugMarkerEnd(buffer);
      }
      SetImageLayout(buffer, mRenderTarget.GetColorImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
      SetImageLayout(buffer, mRenderTarget.GetDepthImage(), VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
      _VulkanManager->DebugMarkerEnd(buffer);
   }

   _VulkanManager->DebugMarkerStart(buffer, "Present Prepare");
   SetImageLayout(buffer, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   VkImageBlit blit{};
   blit.srcOffsets[1].x = mScreenSpaceRT.GetSize().width;
   blit.srcOffsets[1].y = mScreenSpaceRT.GetSize().height;
   blit.srcOffsets[1].z = 1;
   blit.dstOffsets[1].x = mVkManager->GetSwapchainExtent().width;
   blit.dstOffsets[1].y = mVkManager->GetSwapchainExtent().height;
   blit.dstOffsets[1].z = 1;
   blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.srcSubresource.layerCount = 1;
   blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.dstSubresource.layerCount = 1;
   vkCmdBlitImage(buffer, mScreenSpaceRT.GetColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mVkManager->GetPresentImage(frameIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);
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

   mScreenSpaceRT.Destroy();

   mTerrainTestPipeline.Destroy();
   mTerrainTestShadowPipeline.Destroy();
   mTerrainTest.Destroy(); 
   mTerrainAlbedoImages.Destroy();
   mTerrainHeightMapImage.Destroy();
   vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mTerrainTestHeightMapSetLayout, GetAllocationCallback());

   mTerrainSSGPipeline.Destroy();
   mScreenSpaceRT.Destroy();


   {
      VkDescriptorSet sets[] = { mSceneSet, mObjectSet, mMaterialSet };
      //vkFreeDescriptorSets(mVkManager->GetDevice(), mDescriptorPool, 2, sets); //needs VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
      mSceneShadowBuffer.Destroy();
      mSceneBuffer.Destroy();
      mObjectBuffer.Destroy();
      mComputeTestInputBuffer.Destroy();
      mComputeTestOutputBuffer.Destroy();
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mSceneDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mObjectDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mMaterialDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mComputeTestDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mScreenSpaceSetLayout, GetAllocationCallback());
   }

   mRenderTarget.Destroy();
   mRenderPass.Destroy(mVkManager->GetDevice());
   mRenderPassNoDepth.Destroy(mVkManager->GetDevice());
   mBillboardQuad.Destroy();
   mScreenQuad.Destroy();
   mPipelineShadow.Destroy();
   mPipeline.Destroy();
   mPipelineTest.Destroy();
   mComputeTest.Destroy();
   mModelTest.Destroy();

   ShadowManager::Destroy();

   mVkManager->Destroy();
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}
