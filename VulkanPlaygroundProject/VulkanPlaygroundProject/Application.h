#pragma once

#include "Pipeline.h"
#include "Buffer.h"
#include "Model.h"

#include "RenderTarget.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include "Buffer.h"
#include "Image.h"

#include "FlyCamera.h"

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
   void DestroySizeDependent();

   Window* mWindow;
   VulkanManager* mVkManager;

private:
   BufferVertex mScreenQuad;
   BufferVertex mBillboardQuad;
   Pipeline mPipelineTest;
   Pipeline mPipeline;
   Pipeline mPipelineShadow;
   Model mModelTest;

   VkDescriptorSetLayout mSceneDescriptorSet;
   VkDescriptorSetLayout mObjectDescriptorSet;
   VkDescriptorSetLayout mMaterialDescriptorSet;
   VkDescriptorSet mSceneSet;
   VkDescriptorSet mSceneShadowSet;
   VkDescriptorSet mObjectSet;
   VkDescriptorSet mMaterialSet;
   BufferRingUniform mSceneBuffer;
   BufferRingUniform mSceneShadowBuffer;
   BufferRingUniform mObjectBuffer;

   RenderPass mRenderPass;
   RenderTarget mRenderTarget;
   Image mTestImg;

   SceneUBO mSceneUbo{};
   SceneSimpleUBO mSceneShadowUbo{};

   FlyCamera mFlyCamera;

   glm::vec3 mLightPos;
};

