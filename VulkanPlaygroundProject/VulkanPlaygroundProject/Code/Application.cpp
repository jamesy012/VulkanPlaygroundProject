#include "stdafx.h"
#include "Application.h"

#include <imgui.h>

#include "Engine/VulkanManager.h"
#include "Engine/Window.h"
#include "Engine/InputHandler.h"

#include "Engine/RenderManager.h"

#include "Engine/Vertex.h"

#include <Ultralight/Ultralight.h>
#include <AppCore/Platform.h>

ultralight::RefPtr<ultralight::Renderer> renderer;
ultralight::RefPtr<ultralight::View> view;
Image mHtmlUI;
VkDescriptorSet mHtmlUISet;
Pipeline mScreenSpacePipeline;


void CopyBitmapToTexture(Image* image, ultralight::RefPtr<ultralight::Bitmap> bitmap) {
    ///
    /// Lock the Bitmap to retrieve the raw pixels.
    /// The format is BGRA, 8-bpp, premultiplied alpha.
    ///
    bitmap->SwapRedBlueChannels();

    void* pixels = bitmap->LockPixels();

    ///
    /// Get the bitmap dimensions.
    ///
    uint32_t width = bitmap->width();
    uint32_t height = bitmap->height();
    uint32_t stride = bitmap->row_bytes();

    if (image->GetWidth() == width || image->GetHeight() == height/* || image->GetStride() == stride*/) {
        ///
        /// Psuedo-code to upload our pixels to a GPU texture.
        ///
        //CopyPixelsToTexture(pixels, width, height, stride);
        image->SetImageData((const unsigned char*)pixels);
    }
    ///
    /// Unlock the Bitmap when we are done.
    ///
    bitmap->UnlockPixels();
}

void Application::Start() {
   mWindow = new Window();
   mWindow->Create(800, 600, "vulkan");
   mVkManager = new VulkanManager();
   mVkManager->Create(mWindow);
   _CInput = new InputHandler();
   _CInput->Startup(mWindow->GetWindow());


   CreateSizeDependent();
   mVkManager->mSizeDependentCreateCallback = std::bind(&Application::CreateSizeDependent, this);
   mVkManager->mSizeDependentDestroyCallback = std::bind(&Application::DestroySizeDependent, this);

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

   {
       Image::WHITE = new Image();
       const unsigned char data[4] = { 255,255,255,255 };
       Image::WHITE->LoadImageData(1, 1, data);
   }

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
         VkDescriptorSetLayoutBinding bindings[] = { diffuseLayout };
         VkDescriptorSetLayoutCreateInfo layoutInfo{};
         layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = sizeof(bindings)/sizeof(VkDescriptorSetLayoutBinding);
         layoutInfo.pBindings = bindings;
         vkCreateDescriptorSetLayout(mVkManager->GetDevice(), &layoutInfo, GetAllocationCallback(), &mMaterialDescriptorSet);
      }     

      {
         //mPipeline.AddShader(GetWorkDir() + "Shaders/Simple.vert");
         //mPipeline.SetVertexType(VertexTypeDefault);
         mPipeline.AddShader(GetWorkDir() + "Shaders/Simple.vert");
         mPipeline.SetVertexType(VertexTypeDefault);
         mPipeline.AddShader(GetWorkDir() + "Shaders/Textured.frag");
         mPipeline.AddDescriptorSetLayout(mSceneDescriptorSet);
         mPipeline.AddDescriptorSetLayout(mObjectDescriptorSet);
         mPipeline.AddDescriptorSetLayout(mMaterialDescriptorSet);
         mPipeline.SetBlendingEnabled(true);
         mPipeline.Create(mVkManager->GetSwapchainExtent(), &mRenderPass);
      }
      {
          mScreenSpacePipeline.AddShader(GetWorkDir() + "Shaders/ScreenSpace.vert");
          mScreenSpacePipeline.AddShader(GetWorkDir() + "Shaders/Textured.frag");
          mScreenSpacePipeline.SetVertexType(VertexTypeSimple);
          mScreenSpacePipeline.AddDescriptorSetLayout(mSceneDescriptorSet);
          mScreenSpacePipeline.AddDescriptorSetLayout(mObjectDescriptorSet);
          mScreenSpacePipeline.AddDescriptorSetLayout(mMaterialDescriptorSet);
          mScreenSpacePipeline.mDepthTest = false;
          mScreenSpacePipeline.SetBlendingEnabled(true);
          mScreenSpacePipeline.Create(mVkManager->GetSwapchainExtent(), &mRenderPass);
      }

      {
         VkDescriptorSetAllocateInfo setAllocate{};
         setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
         setAllocate.descriptorPool = mVkManager->GetDescriptorPool();
         setAllocate.descriptorSetCount = 1;
         setAllocate.pSetLayouts = &mSceneDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mSceneSet);

         setAllocate.pSetLayouts = &mObjectDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mObjectSet); 
         
         setAllocate.pSetLayouts = &mMaterialDescriptorSet;
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mMaterialSet);
         vkAllocateDescriptorSets(mVkManager->GetDevice(), &setAllocate, &mHtmlUISet);

         mSceneBuffer.Create(3, sizeof(SceneUBO), mSceneSet, 0);
         mSceneBuffer.SetName("Scene Buffer");
         mObjectBuffer.Create(500, sizeof(ObjectUBO), mObjectSet, 0);
         mObjectBuffer.SetName("Object Buffer");

      }
   }

   //mModelTest.LoadModel(GetWorkDir() + "Models/treeTest.fbx", nullptr, {});
   mModelTest.LoadModel(GetWorkDir() + "Sponza/Sponza.obj", mMaterialDescriptorSet, {});
   mModelTest.SetScale(0.05f);
   mModelTest2.LoadModel(GetWorkDir() + "Models/mandalorian-star-wars/source/Mandalorian_Anim_fixed_textures.fbx", mMaterialDescriptorSet, {});
   mModelTest2.SetScale(0.05f);
   mModelTest2.SetRotation(glm::vec3(-90, 0, 0));

   //mFlyCamera.SetPosition(glm::vec3(130, 50, 150));
   mFlyCamera.SetFarClip(150.0f);

   CreateDelayedSizeDependent();

   ultralight::Config config;
   ///
   /// We need to tell config where our resources are so it can 
   /// load our bundled SSL certificates to make HTTPS requests.
   ///
   config.resource_path = "./resources/";
   ///
   /// The GPU renderer should be disabled to render Views to a 
   /// pixel-buffer (Surface).
   ///
   config.use_gpu_renderer = false;
   ///
   /// You can set a custom DPI scale here. Default is 1.0 (100%)
   ///
   config.device_scale = 1.0;
   ///
   /// Pass our configuration to the Platform singleton so that
   /// the library can use it.
   ///
   ultralight::Platform::instance().set_config(config);
   ///
   /// Use the OS's native font loader
   ///
   ultralight::Platform::instance().set_font_loader(ultralight::GetPlatformFontLoader());

   ///
   /// Use the OS's native file loader, with a base directory of "."
   /// All file:/// URLs will load relative to this base directory.
   ///
   //ultralight::Platform::instance().set_file_system(ultralight::GetPlatformFileSystem("."));
   ultralight::Platform::instance().set_file_system(ultralight::GetPlatformFileSystem(GetWorkDir().c_str()));

   ///
   /// Use the default logger (writes to a log file)
   ///
   ultralight::Platform::instance().set_logger(ultralight::GetDefaultLogger("ultralight.log"));

   ///
   /// Create our Renderer (call this only once per application).
   /// 
   /// The Renderer singleton maintains the lifetime of the library
   /// and is required before creating any Views.
   ///
   /// You should set up the Platform handlers before this.
   ///
   renderer = ultralight::Renderer::Create();

   ///
   /// Create an HTML view, 500 by 500 pixels large.
   ///
   view = renderer->CreateView(mVkManager->GetSwapchainExtent().width, mVkManager->GetSwapchainExtent().height, true, nullptr);
   //view = renderer->CreateView(500, 500, false, nullptr);

   ///
   /// Load a raw string of HTML.
   ///
   //view->LoadHTML("<body style=\"background-color: #FFFFFF00;\"><h1>Hello World!</h1></body>");
   //view->LoadURL("file:///F:\\proj\\vulkan\\VulkanPlaygroundProject\\VulkanPlaygroundProject\\WorkDir\\assets\\app.html");
   view->LoadURL("file:///html/app.html");
   //view->LoadURL("https://en.wikipedia.org");
   //view->LoadURL("www.google.com");

   ///
   /// Notify the View it has input focus (updates appearance).
   ///
   view->Focus();

   mHtmlUI.CreateImageEmpty(mVkManager->GetSwapchainExtent().width, mVkManager->GetSwapchainExtent().height);
   VkDescriptorImageInfo info;
   VkWriteDescriptorSet writeSet = GetWriteDescriptorSet(info, mHtmlUI.GetImageLayout(), mHtmlUI.GetImageView(), mHtmlUISet, VulkanManager::Get()->GetDefaultSampler(), 0);
   writeSet.dstSet = mHtmlUISet;
   vkUpdateDescriptorSets(VulkanManager::Get()->GetDevice(), 1, &writeSet, 0, nullptr);

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
   ImGui::End();
}

void Application::Update() {
   PROFILE_START_SCOPED("Update");

   mFlyCamera.UpdateInput();

    //ubo
   {
      //scene
      {
         mSceneUbo.mViewProj = mFlyCamera.GetPV();
         mSceneUbo.mViewPos = glm::vec4(mFlyCamera.GetPostion(), 0);
         //mSceneUbo.mLightPos = glm::vec4(mLightPos, 0);

         //for (int i = 0; i < NUM_SHADOW_CASCADES; i++) {
         //   mSceneUbo.mShadowCascadeProj[i] = shadowOffsetsVpMatrix[i];
         //   glm::value_ptr(mSceneUbo.mShadowSplits)[i] = shadowOffsetsSplitDepth[i];
         //}

         {
            void* data;
            mSceneBuffer.Get(&data);
            memcpy(data, &mSceneUbo, sizeof(SceneUBO));
            mSceneBuffer.Return();
         }
      }
   }

   //mModelTest.SetRotation(glm::vec3(0, frameCounter * 0.07f, 0));

   if (_CInput->GetMouseDelta() != glm::vec2(0, 0)) {
       ultralight::MouseEvent evt;
       evt.type = ultralight::MouseEvent::kType_MouseMoved;
       evt.x = _CInput->GetMousePos().x;
       evt.y = _CInput->GetMousePos().y;
       evt.button = ultralight::MouseEvent::kButton_None;
       view->FireMouseEvent(evt);
   }
   if (_CInput->GetKeysDown().length() != 0) {
       ultralight::KeyEvent  evt;
       evt.type = ultralight::KeyEvent::kType_Char;
       evt.text = _CInput->GetKeysDown().c_str();
       evt.unmodified_text = evt.text;
       view->FireKeyEvent(evt);
   }

   renderer->Update();

}

void Application::CreateSizeDependent() {
   if (!mRenderPass.IsValid()) {
      mRenderPass.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_FORMAT_D32_SFLOAT_S8_UINT);
   }
   if (!mRenderPassNoDepth.IsValid()) {
      mRenderPassNoDepth.Create(mVkManager->GetDevice(), mVkManager->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
   }
   mRenderTarget.Create(mVkManager->GetDevice(), &mRenderPass, mVkManager->GetSwapchainExtent(), true);

   //html resize
   //view->Resize(mVkManager->GetSwapchainExtent().width, mVkManager->GetSwapchainExtent().height);
   //mHtmlUI->Resize(mVkManager->GetSwapchainExtent().width, mVkManager->GetSwapchainExtent().height);
   ////update set?
   //mHtmlUISet

   if (frameCounter != 0) {
      CreateDelayedSizeDependent();
   }
}

void Application::CreateDelayedSizeDependent() {
}

void Application::DestroySizeDependent() {
   mRenderTarget.Destroy();
}


void Application::Draw() {
   PROFILE_START_SCOPED("Draw");
   uint32_t frameIndex;
   VkCommandBuffer commandBuffer;
   if (mVkManager->RenderStart(commandBuffer, frameIndex) == false) {
      ASSERT_RET("Failed to start render");
   }
   frameCounter++;

   {
       renderer->Render();
       ultralight::BitmapSurface* surface = (ultralight::BitmapSurface*)(view->surface());

       ///
       /// Check if our Surface is dirty (pixels have changed).
       ///
       if (!surface->dirty_bounds().IsEmpty()) {
           ///
           /// Psuedo-code to upload Surface's bitmap to GPU texture.
           ///
           CopyBitmapToTexture(&mHtmlUI, surface->bitmap());

           ///
           /// Clear the dirty bounds.
           ///
           surface->ClearDirtyBounds();
       }
   }

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = 0; // Optional
   beginInfo.pInheritanceInfo = nullptr; // Optional
   vkBeginCommandBuffer(commandBuffer, &beginInfo);

   if (mVkManager->DidResizeLastFrame()) {
      for (int i = 0; i < 3; i++) {
         SetImageLayout(commandBuffer, mVkManager->GetPresentImage(i), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      }
   }

   //main render
   {
      VulkanManager::Get()->DebugMarkerStart(commandBuffer, "Main Render", glm::vec4(0.0f, 0.3f, 0.0f, 0.2f));
      std::vector<VkClearValue> clearColor = std::vector<VkClearValue>(2);
      clearColor[0].color.float32[0] = abs(sin((frameCounter * 0.5f) / 5000.0f));
      clearColor[0].color.float32[1] = abs(sin((frameCounter * 0.2f) / 5000.0f));
      clearColor[0].color.float32[2] = abs(sin((frameCounter * 0.1f) / 5000.0f));
      clearColor[0].color.float32[3] = 1.0f;
      clearColor[1].depthStencil.depth = 1.0f;
      clearColor[1].depthStencil.stencil = 0;
      mRenderTarget.StartRenderPass(commandBuffer, clearColor);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipeline());

      uint32_t descriptorSetOffsets[] = { mSceneBuffer.GetCurrentOffset() };
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.GetPipelineLayout(), 0, 1, &mSceneSet, 1, descriptorSetOffsets);

      //render Light facing camera
      {
         DescriptorUBO des = DescriptorUBO(commandBuffer, mPipeline.GetPipelineLayout(), &mObjectBuffer, mObjectSet);
         ObjectUBO ubo;
         ubo.mModel = mModelTest.GetMatrix();
         //ubo.mModel = glm::rotate(mModelTest.GetMatrix(), abs(sin((frameCounter * 0.5f) / 5000.0f)), glm::vec3(0,1,0));
         des.UpdateObjectAndBind(&ubo);
      
         mModelTest.Render(&des, RenderMode::NORMAL);

         ubo.mModel = mModelTest2.GetMatrix();
         des.UpdateObjectAndBind(&ubo);
         mModelTest2.Render(&des, RenderMode::NORMAL);
      
      }
      //RenderManager::Get()->Render( commandBuffer );
      //vkCmdDrawIndexedIndirect(commandBuffer, renderIndirectBufferOUT.GetBuffer(), 0, renderIndirectBufferOUT.GetCount(), sizeof( VkDrawIndexedIndirectCommand ) );
      //vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
      //vkCmdDrawIndexedIndirectCount(commandBuffer, buffer, offset, countbuffer, countbufferoffset, maxDrawCount, stride);
      //vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOFfset, firstInstance);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mScreenSpacePipeline.GetPipeline());
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mScreenSpacePipeline.GetPipelineLayout(), 2, 1, &mHtmlUISet, 0, nullptr);
      mScreenQuad.Bind(commandBuffer);
      vkCmdDraw(commandBuffer, 6, 1, 0, 0);

      mRenderTarget.EndRenderPass(commandBuffer);
      VulkanManager::Get()->DebugMarkerEnd(commandBuffer);
   }

   //copy renderTarget to the back buffer
   {
      VulkanManager::Get()->BlitRenderTargetToBackBuffer(commandBuffer, &mRenderTarget);
   }

   vkEndCommandBuffer(commandBuffer);

   mVkManager->RenderEnd();
}


void Application::Destroy() {
   _CInput->Shutdown();
   delete _CInput;
   mVkManager->WaitDevice();

   mHtmlUI.Destroy();
   mScreenSpacePipeline.Destroy();

   {
      mSceneBuffer.Destroy();
      mObjectBuffer.Destroy();
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mMaterialDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mSceneDescriptorSet, GetAllocationCallback());
      vkDestroyDescriptorSetLayout(mVkManager->GetDevice(), mObjectDescriptorSet, GetAllocationCallback());
   }

   mRenderTarget.Destroy();
   mRenderPass.Destroy(mVkManager->GetDevice());
   mRenderPassNoDepth.Destroy(mVkManager->GetDevice());
   mBillboardQuad.Destroy();
   mScreenQuad.Destroy();
   mPipeline.Destroy();
   mModelTest.Destroy();
   mModelTest2.Destroy();

   Image::WHITE->Destroy();
   delete Image::WHITE;

   mVkManager->Destroy();
   mWindow->Destroy();
   delete mWindow;
   delete mVkManager;
}
