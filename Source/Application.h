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
   void Start(int argc, char *argv[]);
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
   Pipeline mPipeline;
   Model mModelTest;
   Model mModelTest2;

   VkDescriptorSetLayout mSceneDescriptorSet;
   VkDescriptorSetLayout mObjectDescriptorSet;
   VkDescriptorSetLayout mMaterialDescriptorSet;
   VkDescriptorSet mSceneSet;
   VkDescriptorSet mObjectSet;
   VkDescriptorSet mMaterialSet;
   BufferRingUniform mSceneBuffer;
   BufferRingUniform mObjectBuffer;

   RenderPass mRenderPass;
   RenderPass mRenderPassNoDepth;
   RenderTarget mRenderTarget;

   SceneUBO mSceneUbo{};

   FlyCamera mFlyCamera;
};

