#include "stdafx.h"
#include "Browser.h"

#include <include/cef_app.h>
#include <include/cef_browser.h>
//#include <include/cef_sandbox_win.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

void BrowserRender::init(void) {
   initialized = true;
}

void BrowserRender::draw(void) {
}

void BrowserRender::reshape(int, int) {
}

void BrowserRender::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) {

}

void BrowserRender::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
}

void initCefgui(int argc, char** argv) {

   void* sandbox_info = nullptr;
   //// Manage the life span of the sandbox information object. This is necessary
   //// for sandbox support on Windows. See cef_sandbox_win.h for complete details.
   //CefScopedSandboxInfo scoped_sandbox;
   //sandbox_info = scoped_sandbox.sandbox_info();

   HINSTANCE hInstance = GetModuleHandle(NULL);
   CefMainArgs args(hInstance);
   int result = CefExecuteProcess(args, nullptr, sandbox_info);
   if (result >= 0) // child proccess has endend, so exit.
   {
      return;
   }
   if (result == -1) {
      // we are here in the father proccess.
   }

   CefSettings settings;
   settings.windowless_rendering_enabled = true;
   settings.no_sandbox = true;
   settings.multi_threaded_message_loop = false;
   settings.persist_session_cookies = false;
   CefString(&settings.user_agent).FromASCII("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2125.104 Safari/537.36");
   CefString(&settings.cache_path).FromASCII("");

   result = CefInitialize(args, settings, nullptr, sandbox_info);
   if (!result) {
      // handle error
      return;
   }
}

Browser::Browser(GLFWwindow* a_InputWindow) {
   initCefgui(0, nullptr);

   CefWindowInfo windowInfo;
   CefBrowserSettings settings;

   HWND hwnd = glfwGetWin32Window(a_InputWindow);
   windowInfo.SetAsWindowless(hwnd);

   windowInfo.windowless_rendering_enabled = true;
   CefString(&windowInfo.window_name).FromASCII("windowless");

   renderHandler = new BrowserRender();

   client = new BrowserClient(renderHandler);
   browser = CefBrowserHost::CreateBrowserSync(windowInfo, client.get(), "http://deanm.github.io/pre3d/monster.html", settings, nullptr, nullptr);

   reshape(1000, 1000);
   CefRect rect;
   renderHandler->GetViewRect(browser, rect);

}

void Browser::load(const char* url) {
   if (!renderHandler->initialized) {
      renderHandler->init();
   }
   //browser->GetMainFrame()->LoadURL("http://www.google.com");
   browser->GetMainFrame()->LoadURL("file:///D:/coding/websites/drawGame/index.html");
}

void Browser::draw(void) {
   CefDoMessageLoopWork();
   renderHandler->draw();
}

void Browser::reshape(int w, int h) {
   renderHandler->reshape(w, h);
   browser->GetHost()->WasResized();
}

void Browser::mouseMove(int x, int y) {
   mouseX = x;
   mouseY = y;

   CefMouseEvent event;
   event.x = x;
   event.y = y;

   browser->GetHost()->SendMouseMoveEvent(event, false);
}

void Browser::mouseClick(int btn, int state) {
   CefMouseEvent event;
   event.x = mouseX;
   event.y = mouseY;

   bool mouseUp = state == 0;
   CefBrowserHost::MouseButtonType btnType = MBT_LEFT;
   browser->GetHost()->SendMouseClickEvent(event, btnType, mouseUp, 1);
}

void Browser::keyPress(int key) {
   CefKeyEvent event;
   event.native_key_code = key;
   event.type = KEYEVENT_KEYDOWN;

   browser->GetHost()->SendKeyEvent(event);
}

void Browser::executeJS(const char* command) {
   CefRefPtr<CefFrame> frame = browser->GetMainFrame();
   frame->ExecuteJavaScript(command, frame->GetURL(), 0);

   // TODO limit frequency of texture updating
   //CefRect rect;
   //renderHandler->GetViewRect(browser, rect);
   browser->GetHost()->Invalidate(PET_VIEW);
}

BrowserClient::BrowserClient(BrowserRender* renderHandler) {
   handler = renderHandler;
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
   return handler;
}

//~~~~~~~~~~~~~~~~~~~~

// Implement application-level callbacks for the browser process.
class SimpleApp : public CefApp, public CefBrowserProcessHandler {
public:
   SimpleApp();

   // CefApp methods:
   CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE {
      return this;
   }

   // CefBrowserProcessHandler methods:
   void OnContextInitialized() OVERRIDE;
   CefRefPtr<CefClient> GetDefaultClient() OVERRIDE;

   HWND hwnd = 0;
private:
   // Include the default reference counting implementation.
   IMPLEMENT_REFCOUNTING(SimpleApp);
};

class SimpleHandler : public CefClient,
   public CefDisplayHandler,
   public CefLifeSpanHandler,
   public CefLoadHandler {
public:
   explicit SimpleHandler(bool use_views);
   ~SimpleHandler();

   // Provide access to the single global instance of this object.
   static SimpleHandler* GetInstance();

   // CefClient methods:
   virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE {
      return this;
   }
   virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
      return this;
   }
   virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }

   // CefDisplayHandler methods:
   virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
      const CefString& title) OVERRIDE;

   // CefLifeSpanHandler methods:
   virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
   virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
   virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

   // CefLoadHandler methods:
   virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      ErrorCode errorCode,
      const CefString& errorText,
      const CefString& failedUrl) OVERRIDE;

   // Request that all existing browser windows close.
   void CloseAllBrowsers(bool force_close);

   bool IsClosing() const { return is_closing_; }

   // Returns true if the Chrome runtime is enabled.
   static bool IsChromeRuntimeEnabled();

   virtual CefRefPtr<CefRenderHandler> GetRenderHandler() {
      return rh;
   }
   CefRefPtr<CefRenderHandler> rh = nullptr;

private:
   // Platform-specific implementation.
   void PlatformTitleChange(CefRefPtr<CefBrowser> browser,
      const CefString& title);

   // True if the application is using the Views framework.
   const bool use_views_;

   // List of existing browser windows. Only accessed on the CEF UI thread.
   typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
   BrowserList browser_list_;

   bool is_closing_;

   // Include the default reference counting implementation.
   IMPLEMENT_REFCOUNTING(SimpleHandler);
};

//~~~~~~~~~~~~~~~~~~~~
 #include "include/cef_browser.h"

void SimpleHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser,
   const CefString& title) {
   CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
   if (hwnd)
      SetWindowText(hwnd, std::wstring(title).c_str());
}
//~~~~~~~~~~~~~~~~~~~~

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

namespace {

   // When using the Views framework this object provides the delegate
   // implementation for the CefWindow that hosts the Views-based browser.
   class SimpleWindowDelegate : public CefWindowDelegate {
   public:
      explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
         : browser_view_(browser_view) {
      }

      void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
         // Add the browser view and show the window.
         window->AddChildView(browser_view_);
         window->Show();

         // Give keyboard focus to the browser view.
         browser_view_->RequestFocus();
      }

      void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
         browser_view_ = nullptr;
      }

      bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
         // Allow the window to close if the browser says it's OK.
         CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
         if (browser)
            return browser->GetHost()->TryCloseBrowser();
         return true;
      }

      CefSize GetPreferredSize(CefRefPtr<CefView> view) OVERRIDE {
         return CefSize(800, 600);
      }

   private:
      CefRefPtr<CefBrowserView> browser_view_;

      IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
      DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
   };

   class SimpleBrowserViewDelegate : public CefBrowserViewDelegate {
   public:
      SimpleBrowserViewDelegate() {}

      bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view,
         CefRefPtr<CefBrowserView> popup_browser_view,
         bool is_devtools) OVERRIDE {
         // Create a new top-level Window for the popup. It will show itself after
         // creation.
         CefWindow::CreateTopLevelWindow(
            new SimpleWindowDelegate(popup_browser_view));

         // We created the Window.
         return true;
      }

   private:
      IMPLEMENT_REFCOUNTING(SimpleBrowserViewDelegate);
      DISALLOW_COPY_AND_ASSIGN(SimpleBrowserViewDelegate);
   };

}  // namespace

SimpleApp::SimpleApp() {}

void SimpleApp::OnContextInitialized() {
   CEF_REQUIRE_UI_THREAD();

   CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

   // Create the browser using the Views framework if "--use-views" is specified
   // via the command-line. Otherwise, create the browser using the native
   // platform framework.
   const bool use_views = command_line->HasSwitch("use-views");
   RenderHandler* rh = new RenderHandler();
   // SimpleHandler implements browser-level callbacks.
   CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views));

   handler->rh = rh;

   // Specify CEF browser settings here.
   CefBrowserSettings browser_settings;

   std::string url;

   // Check if a "--url=" value was provided via the command-line. If so, use
   // that instead of the default URL.
   url = command_line->GetSwitchValue("url");
   if (url.empty())
      url = "http://www.google.com";

   if (use_views) {
      // Create the BrowserView.
      CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
         handler, url, browser_settings, nullptr, nullptr,
         new SimpleBrowserViewDelegate());

      // Create the Window. It will show itself after creation.
      CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
   } else {
      // Information used when creating the native window.
      CefWindowInfo window_info;

#if defined(OS_WIN)
      // On Windows we need to specify certain flags that will be passed to
      // CreateWindowEx().
      window_info.SetAsPopup(NULL, "cefsimple");
#endif
      if (hwnd != 0) {
         window_info.SetAsWindowless(hwnd);
      }

      // Create the first browser window.
      CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings,
         nullptr, nullptr);
   }
}

CefRefPtr<CefClient> SimpleApp::GetDefaultClient() {
   // Called when a new browser window is created via the Chrome runtime UI.
   return SimpleHandler::GetInstance();
}


//~~~~~~~~~~~~~~~~~~~~
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {

   SimpleHandler* g_instance = nullptr;

   // Returns a data: URI with the specified contents.
   std::string GetDataURI(const std::string& data, const std::string& mime_type) {
      return "data:" + mime_type + ";base64," +
         CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
         .ToString();
   }

}  // namespace

SimpleHandler::SimpleHandler(bool use_views)
   : use_views_(use_views), is_closing_(false) {
   DCHECK(!g_instance);
   g_instance = this;
}

SimpleHandler::~SimpleHandler() {
   g_instance = nullptr;
}

// static
SimpleHandler* SimpleHandler::GetInstance() {
   return g_instance;
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
   const CefString& title) {
   CEF_REQUIRE_UI_THREAD();

   if (use_views_) {
      // Set the title of the window using the Views framework.
      CefRefPtr<CefBrowserView> browser_view =
         CefBrowserView::GetForBrowser(browser);
      if (browser_view) {
         CefRefPtr<CefWindow> window = browser_view->GetWindow();
         if (window)
            window->SetTitle(title);
      }
   } else if (!IsChromeRuntimeEnabled()) {
      // Set the title of the window using platform APIs.
      PlatformTitleChange(browser, title);
   }
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
   CEF_REQUIRE_UI_THREAD();

   // Add to the list of existing browsers.
   browser_list_.push_back(browser);
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
   CEF_REQUIRE_UI_THREAD();

   // Closing the main window requires special handling. See the DoClose()
   // documentation in the CEF header for a detailed destription of this
   // process.
   if (browser_list_.size() == 1) {
      // Set a flag to indicate that the window close should be allowed.
      is_closing_ = true;
   }

   // Allow the close. For windowed browsers this will result in the OS close
   // event being sent.
   return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
   CEF_REQUIRE_UI_THREAD();

   // Remove from the list of existing browsers.
   BrowserList::iterator bit = browser_list_.begin();
   for (; bit != browser_list_.end(); ++bit) {
      if ((*bit)->IsSame(browser)) {
         browser_list_.erase(bit);
         break;
      }
   }

   if (browser_list_.empty()) {
      // All browser windows have closed. Quit the application message loop.
      CefQuitMessageLoop();
   }
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
   CefRefPtr<CefFrame> frame,
   ErrorCode errorCode,
   const CefString& errorText,
   const CefString& failedUrl) {
   CEF_REQUIRE_UI_THREAD();

   // Allow Chrome to show the error page.
   if (IsChromeRuntimeEnabled())
      return;

   // Don't display an error for downloaded files.
   if (errorCode == ERR_ABORTED)
      return;

   // Display a load error message using a data: URI.
   std::stringstream ss;
   ss << "<html><body bgcolor=\"white\">"
      "<h2>Failed to load URL "
      << std::string(failedUrl) << " with error " << std::string(errorText)
      << " (" << errorCode << ").</h2></body></html>";

   frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void SimpleHandler::CloseAllBrowsers(bool force_close) {
   if (!CefCurrentlyOn(TID_UI)) {
      // Execute on the UI thread.
      CefPostTask(TID_UI, base::Bind(&SimpleHandler::CloseAllBrowsers, this,
         force_close));
      return;
   }

   if (browser_list_.empty())
      return;

   BrowserList::const_iterator it = browser_list_.begin();
   for (; it != browser_list_.end(); ++it)
      (*it)->GetHost()->CloseBrowser(force_close);
}

// static
bool SimpleHandler::IsChromeRuntimeEnabled() {
   static int value = -1;
   if (value == -1) {
      CefRefPtr<CefCommandLine> command_line =
         CefCommandLine::GetGlobalCommandLine();
      value = command_line->HasSwitch("enable-chrome-runtime") ? 1 : 0;
   }
   return value == 1;
}

//~~~~~~~~~~~~~~~~~~~~

int BrowserStart(GLFWwindow* aWindow, bool quickTest) {
   CefEnableHighDPISupport();

   HINSTANCE hInstance = GetModuleHandle(NULL);
   //CefMainArgs args(argc, argv);
   CefMainArgs args(hInstance);

   CefRefPtr<SimpleApp> app(new SimpleApp);

   HWND hwnd = glfwGetWin32Window(aWindow);
   app->hwnd = hwnd;

   void* sandbox_info = nullptr;

   {
      int result = CefExecuteProcess(args, app, sandbox_info);
      // checkout CefApp, derive it and set it as second parameter, for more control on
      // command args and resources.
      if (result >= 0) // child proccess has endend, so exit.
      {
         return result;
      }
      if (result == -1) {
         // we are here in the father proccess.
      }
   }

   if (quickTest) {
      return -1;
   }

   {
      CefSettings settings;

      // checkout detailed settings options http://magpcss.org/ceforum/apidocs/projects/%28default%29/_cef_settings_t.html
      // nearly all the settings can be set via args too.
      // settings.multi_threaded_message_loop = true; // not supported, except windows
      // CefString(&settings.browser_subprocess_path).FromASCII("sub_proccess path, by default uses and starts this executeable as child");
      // CefString(&settings.cache_path).FromASCII("");
      // CefString(&settings.log_file).FromASCII("");
      // settings.log_severity = LOGSEVERITY_DEFAULT;
      // CefString(&settings.resources_dir_path).FromASCII("");
      // CefString(&settings.locales_dir_path).FromASCII("");
      settings.windowless_rendering_enabled = true;

      bool result = CefInitialize(args, settings, app, nullptr);
      // CefInitialize creates a sub-proccess and executes the same executeable, as calling CefInitialize, if not set different in settings.browser_subprocess_path
      // if you create an extra program just for the childproccess you only have to call CefExecuteProcess(...) in it.
      if (!result)         {
         // handle error
         return -1;
      }
   }

   //RenderHandler* renderHandler;
   //{
   //   //renderHandler = new RenderHandler(renderTexture, renderNode);
   //   renderHandler = new RenderHandler();
   //
   //   //renderSystem->addFrameListener(renderHandler);
   //}

   //// create browser-window
   //CefRefPtr<CefBrowser> browser;
   //CefRefPtr<BrowserClient2> browserClient;
   //{
   //   CefWindowInfo window_info;
   //   CefBrowserSettings browserSettings;
   //
   //    browserSettings.windowless_frame_rate = 60; // 30 is default
   //
   //   // in linux set a gtk widget, in windows a hwnd. If not available set nullptr - may cause some render errors, in context-menu and plugins.
   //   //std::size_t windowHandle = 0;
   //   //renderSystem->getAutoCreatedWindow()->getCustomAttribute("WINDOW", &windowHandle);
   //   //window_info.SetAsWindowless(windowHandle, false); // false means no transparency (site background colour)
   //   HWND hwnd = glfwGetWin32Window(aWindow);
   //   window_info.SetAsWindowless(hwnd);
   //
   //
   //   browserClient = new BrowserClient2(renderHandler);
   //
   //   browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), "http://deanm.github.io/pre3d/monster.html", browserSettings, nullptr, nullptr);
   //
   //   // inject user-input by calling - non-trivial for non-windows - checkout the cefclient source and the platform specific cpp, like cefclient_osr_widget_gtk.cpp for linux
   //   // browser->GetHost()->SendKeyEvent(...);
   //   // browser->GetHost()->SendMouseMoveEvent(...);
   //   // browser->GetHost()->SendMouseClickEvent(...);
   //   // browser->GetHost()->SendMouseWheelEvent(...);
   //}
   //CefRunMessageLoop();

   // start rendering and calling method RenderHandler::frameStarted
   {
      //renderSystem->startRendering();
   }

   //{
   //   browser = nullptr;
   //   browserClient = nullptr;
   //   CefShutdown();
   //
   //   //renderScene->destroyAllMovableObjects();
   //   //delete renderScene;
   //   //delete renderSystem;
   //   delete renderHandler;
   //}

   return 1;

}
