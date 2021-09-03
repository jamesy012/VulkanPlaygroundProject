#pragma once

#include "Engine/Pipeline.h"
#include "Engine/Buffer.h"
#include "Engine/Model.h"

#include "Engine/RenderTarget.h"
#include "Engine/Framebuffer.h"
#include "Engine/RenderPass.h"
#include "Engine/Buffer.h"
#include "Engine/Image.h"

#include "Engine/FlyCamera.h"

class Window;
class VulkanManager;

class Application {
public:
   void Start();
   void Run();
   void Destroy();
private:
   void Draw();
   void ImGui();
   void Update();

   void CreateSizeDependent();
   //size dependent Resources that need other systems (first frame?) to be created first
   void CreateDelayedSizeDependent();
   void DestroySizeDependent();

   Window* mWindow;
   VulkanManager* mVkManager;

private:
   BufferVertex mScreenQuad;
   BufferVertex mBillboardQuad;
   Pipeline mPipelineTest;
   Pipeline mPipeline;
   Pipeline mPipelineShadow;
   Pipeline mComputeTest;
   Model mModelTest;

   VkDescriptorSetLayout mSceneDescriptorSet;
   VkDescriptorSetLayout mObjectDescriptorSet;
   VkDescriptorSetLayout mMaterialDescriptorSet;
   VkDescriptorSetLayout mComputeTestDescriptorSet;
   VkDescriptorSet mSceneSet;
   VkDescriptorSet mSceneShadowSet;
   VkDescriptorSet mObjectSet;
   VkDescriptorSet mMaterialSet;
   VkDescriptorSet mComputeTestSet;
   BufferRingUniform mSceneBuffer;
   BufferRingUniform mSceneShadowBuffer;
   BufferRingUniform mObjectBuffer;
   BufferStorageUniform mComputeTestInputBuffer;
   BufferStorageUniform mComputeTestOutputBuffer;

   RenderPass mRenderPass;
   RenderPass mRenderPassNoDepth;
   RenderTarget mRenderTarget;
   Image mTestImg;

   SceneUBO mSceneUbo{};
   SceneSimpleUBO mSceneShadowUbo{};

   FlyCamera mFlyCamera;

   glm::vec3 mLightPos;
};

