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
   Model mModelTest;

   VkDescriptorSetLayout mSceneDescriptorSet;
   VkDescriptorSetLayout mObjectDescriptorSet;
   VkDescriptorSetLayout mMaterialDescriptorSet;
   VkDescriptorSet mSceneSet;
   VkDescriptorSet mObjectSet;
   VkDescriptorSet mMaterialSet;
   BufferRingUniform mSceneBuffer;
   BufferRingUniform mObjectBuffer;

   RenderPass mRenderPass;
   RenderTarget mRenderTarget;
   Image mTestImg;

   SceneUBO mSceneUbo{};

   FlyCamera mFlyCamera;

   glm::vec3 mLightPos;
};

