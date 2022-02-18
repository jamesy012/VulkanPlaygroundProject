#include "stdafx.h"
#include "WebUI.h"

#include "Engine/VulkanManager.h"
#include "Engine/InputHandler.h"
#include "Engine/Image.h"
#include "Engine/Pipeline.h"
#include "Engine/Vertex.h"

#if WEB_UI_ENABLED == 1
#include <Ultralight/Ultralight.h>
#include <AppCore/Platform.h>

ultralight::RefPtr<ultralight::Renderer> renderer;
ultralight::RefPtr<ultralight::View> view;
#endif

Image mHtmlUI;
VkDescriptorSet mHtmlUISet;
Pipeline mScreenSpacePipeline;

#if WEB_UI_ENABLED == 1
JSValueRef OnButtonClick(JSContextRef ctx, JSObjectRef function,
    JSObjectRef thisObject, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception) {
    static int counter = 0;

    std::string str =
        "document.getElementById('time').innerText = '";
    str += std::to_string(++counter);
    str += "'";

    // Create our string of JavaScript
    JSStringRef script = JSStringCreateWithUTF8CString(str.c_str());

    // Execute it with JSEvaluateScript, ignoring other parameters for now
    JSEvaluateScript(ctx, script, 0, 0, 0, 0);

    // Release our string (we only Release what we Create)
    JSStringRelease(script);

    return JSValueMakeNull(ctx);
}

void WebUI::JavaScriptTest(const ultralight::JSObject& thisObject, const ultralight::JSArgs& args) {
    for (int i = 0; i < args.size(); i++) {
        auto arg = args[i];
        ultralight::String string = arg.ToString();
        LOG("%s", string.utf16().data());
    }
    static int counter = 0;

    std::string str =
        "document.getElementById('time').innerText = '";
    str += std::to_string(++counter);
    str += "'";

    // Create our string of JavaScript
    JSStringRef script = JSStringCreateWithUTF8CString(str.c_str());

    // Execute it with JSEvaluateScript, ignoring other parameters for now
    JSEvaluateScript(thisObject.context(), script, 0, 0, 0, 0);

    // Release our string (we only Release what we Create)
    JSStringRelease(script);

}

ultralight::JSValue WebUI::JavaScriptTestRet(const ultralight::JSObject& thisObject, const ultralight::JSArgs& args) {
    for (int i = 0; i < args.size(); i++) {
        auto arg = args[i];
        ultralight::String string = arg.ToString();
        LOG("%s", string.utf8().data());
    }
    static int counter = 0;

    return ultralight::JSValue(++counter);

}

void CopyBitmapToTexture(Image* image, ultralight::IntRect bounds, ultralight::RefPtr<ultralight::Bitmap> bitmap) {
    bitmap->SwapRedBlueChannels();

    ///
    /// Lock the Bitmap to retrieve the raw pixels.
    /// The format is BGRA, 8-bpp, premultiplied alpha.
    ///

    void* pixels = bitmap->LockPixels();

    ///
    /// Get the bitmap dimensions.
    ///
    uint32_t width = bitmap->width();
    uint32_t height = bitmap->height();
    uint32_t stride = bitmap->row_bytes();
    ultralight::BitmapFormat format = bitmap->format();

    if (image->GetWidth() == width || image->GetHeight() == height/* || image->GetStride() == stride*/) {
        ///
        /// Psuedo-code to upload our pixels to a GPU texture.
        ///
        //CopyPixelsToTexture(pixels, width, height, stride);
        //image->SetImageData((const unsigned char*)pixels);
        image->SetImageData(VkExtent2D(bounds.left, bounds.top), VkExtent2D(bounds.width(), bounds.height()), (const unsigned char*)pixels);
    }
    ///
    /// Unlock the Bitmap when we are done.
    ///
    bitmap->UnlockPixels();
}
#endif

void WebUI::SetupMaterials(std::vector<VkDescriptorSetLayout> aSetLayouts, const RenderPass* aRenderPass)
{
#if WEB_UI_ENABLED == 1
    mScreenSpacePipeline.AddShader(GetWorkDir() + "Shaders/ScreenSpace.vert");
    mScreenSpacePipeline.AddShader(GetWorkDir() + "Shaders/Textured.frag");
    mScreenSpacePipeline.SetVertexType(VertexTypeSimple);
    mScreenSpacePipeline.AddDescriptorSetLayout(aSetLayouts);
    mScreenSpacePipeline.mDepthTest = false;
    mScreenSpacePipeline.SetBlendingEnabled(true);
    mScreenSpacePipeline.Create(VulkanManager::Get()->GetSwapchainExtent(), aRenderPass);
#endif
}

void WebUI::Start()
{
#if WEB_UI_ENABLED == 1
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
    view = renderer->CreateView(VulkanManager::Get()->GetSwapchainExtent().width, VulkanManager::Get()->GetSwapchainExtent().height, true, nullptr);
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


    ultralight::Ref<ultralight::JSContext> context = view->LockJSContext();
    ultralight::SetJSContext(context.get());
    ultralight::JSObject global = ultralight::JSGlobalObject();
    //global["OnButtonClick"] = (ultralight::JSCallback)std::bind(&Application::JavaScriptTest, this, std::placeholders::_1, std::placeholders::_2);
    global["OnButtonClick"] = (ultralight::JSCallbackWithRetval)std::bind(&WebUI::JavaScriptTestRet, this, std::placeholders::_1, std::placeholders::_2);

    //// Get the underlying JSContextRef for use with the
    //// JavaScriptCore C API.
    //JSContextRef ctx = context.get();
    //
    //// Create a JavaScript String containing the name of our callback.
    //JSStringRef name = JSStringCreateWithUTF8CString("OnButtonClick");
    //// Create a garbage-collected JavaScript function that is bound to our
    //// native C callback 'OnButtonClick()'.
    //JSObjectRef func = JSObjectMakeFunctionWithCallback(ctx, name,
    //    OnButtonClick);
    //
    //// Get the global JavaScript object (aka 'window')
    //JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
    //
    //// Store our function in the page's global JavaScript object so that it
    //// accessible from the page as 'OnButtonClick()'.
    //JSObjectSetProperty(ctx, globalObj, name, func, 0, 0);
    //
    //// Release the JavaScript String we created earlier.
    //JSStringRelease(name);

    mHtmlUI.CreateImageEmpty(VulkanManager::Get()->GetSwapchainExtent().width, VulkanManager::Get()->GetSwapchainExtent().height);
    VkDescriptorImageInfo info;
    VkWriteDescriptorSet writeSet = GetWriteDescriptorSet(info, mHtmlUI.GetImageLayout(), mHtmlUI.GetImageView(), mHtmlUISet, VulkanManager::Get()->GetDefaultSampler(), 0);
    writeSet.dstSet = mHtmlUISet;
    vkUpdateDescriptorSets(VulkanManager::Get()->GetDevice(), 1, &writeSet, 0, nullptr);
#endif
}

void WebUI::Update()
{
#if WEB_UI_ENABLED == 1
    //mModelTest.SetRotation(glm::vec3(0, frameCounter * 0.07f, 0));
    {//mouse
        ultralight::MouseEvent evt;
        evt.x = (int)_CInput->GetMousePos().x;
        evt.y = (int)_CInput->GetMousePos().y;
        const ultralight::MouseEvent::Button remap[] = { ultralight::MouseEvent::kButton_Left,ultralight::MouseEvent::kButton_Right, ultralight::MouseEvent::kButton_Middle };
        for (int i = 0; i < 3; i++) {
            evt.button = remap[i];
            if (_CInput->WasMouseKeyPressed((IMouseKeys)i)) {
                evt.type = ultralight::MouseEvent::kType_MouseDown;
                view->FireMouseEvent(evt);
                LOG("Down %i\n", i);
            }
            if (_CInput->WasMouseKeyReleased((IMouseKeys)i)) {
                evt.type = ultralight::MouseEvent::kType_MouseUp;
                view->FireMouseEvent(evt);
                LOG("Up %i\n", i);
            }
        }
        if (_CInput->GetMouseDelta() != glm::vec2(0, 0)) {
            evt.type = ultralight::MouseEvent::kType_MouseMoved;
            evt.button = ultralight::MouseEvent::kButton_None;
            view->FireMouseEvent(evt);
        }
    }
    std::vector<char> keysDown = _CInput->GetKeysDownArray();
    char keyValue[] = { 0,0 };//add a 0 terminator to the end?
    for (int i = 0; i < keysDown.size(); i++) {
        ultralight::KeyEvent  evt;
        evt.type = ultralight::KeyEvent::kType_Char;//kType_RawKeyDown?
        keyValue[0] = keysDown[i];
        evt.text = keyValue;
        evt.unmodified_text = evt.text;
        view->FireKeyEvent(evt);
    }

    renderer->Update();
#endif
}

void WebUI::Render(VkCommandBuffer aCommandBuffer, BufferVertex* aScreenSpaceQuad)
{
#if WEB_UI_ENABLED == 1
    renderer->Render();
    ultralight::BitmapSurface* surface = (ultralight::BitmapSurface*)(view->surface());

    ///
    /// Check if our Surface is dirty (pixels have changed).
    ///
    if (!surface->dirty_bounds().IsEmpty()) {
        ///
        /// Psuedo-code to upload Surface's bitmap to GPU texture.
        ///
        CopyBitmapToTexture(&mHtmlUI, surface->dirty_bounds(), surface->bitmap());

        ///
        /// Clear the dirty bounds.
        ///
        surface->ClearDirtyBounds();
    }

    vkCmdBindPipeline(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mScreenSpacePipeline.GetPipeline());
    vkCmdBindDescriptorSets(aCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mScreenSpacePipeline.GetPipelineLayout(), 2, 1, &mHtmlUISet, 0, nullptr);
    aScreenSpaceQuad->Bind(aCommandBuffer);
    vkCmdDraw(aCommandBuffer, 6, 1, 0, 0);
#endif
}

void WebUI::Destroy()
{
#if WEB_UI_ENABLED == 1
    mHtmlUI.Destroy();
    mScreenSpacePipeline.Destroy();
#endif
}

VkDescriptorSet& WebUI::GetDescriptorSet()
{
    return mHtmlUISet;
}
