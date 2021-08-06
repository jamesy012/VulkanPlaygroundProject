#pragma once

#include <include/cef_render_handler.h>

class BrowserRender : public CefRenderHandler {
public:

   void init(void);
   void draw(void);
   void reshape(int, int);

   bool initialized = false;

   // Inherited via CefRenderHandler
   void OnPaint(CefRefPtr<CefBrowser> browser,
      PaintElementType type,
      const RectList& dirtyRects,
      const void* buffer,
      int width,
      int height);

   virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

private:
   IMPLEMENT_REFCOUNTING(BrowserRender);
};

#include <include/cef_client.h>

class BrowserClient : public CefClient {
public:
   BrowserClient(BrowserRender*);

   virtual CefRefPtr<CefRenderHandler> GetRenderHandler();
private:
   CefRefPtr<CefRenderHandler> handler;


   IMPLEMENT_REFCOUNTING(BrowserClient);
};

#include <GLFW/glfw3.h>

class Browser {
public:
   Browser(GLFWwindow* a_InputWindow);

   void load(const char*);
   void draw(void);
   void reshape(int, int);

   void mouseMove(int, int);
   void mouseClick(int, int);
   void keyPress(int);

   void executeJS(const char*);

private:
   int mouseX, mouseY;

   CefRefPtr<CefBrowser> browser;
   CefRefPtr<BrowserClient> client;

   BrowserRender* renderHandler;
};


#include <include/cef_app.h>
#include <include/cef_browser.h>

int BrowserStart(GLFWwindow* aWindow, bool quickTest = false);
static void BrowserUpdate() {
   CefDoMessageLoopWork();
}

class RenderHandler : public CefRenderHandler {
public:

   RenderHandler() {
   }

   // FrameListener interface
public:
   bool frameStarted() {

      //m_renderNode->yaw(Ogre::Radian(evt.timeSinceLastFrame) * Ogre::Math::PI * 2. * (1. / 10.)); // one turn in 10sec

      CefDoMessageLoopWork();

      return true;
   }

   // CefRenderHandler interface
public:
   void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
      rect = CefRect(0, 0, 1000, 1000);//m_renderTexture->getWidth(), m_renderTexture->getHeight());
   }
   void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) {
      //Ogre::HardwarePixelBufferSharedPtr texBuf = m_renderTexture->getBuffer();
      //texBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD);
      //memcpy(texBuf->getCurrentLock().data, buffer, width * height * 4);
      //texBuf->unlock();
   }

   // CefBase interface
public:
   IMPLEMENT_REFCOUNTING(RenderHandler);
};


// for manual render handler
class BrowserClient2 : public CefClient {
public:
   BrowserClient2(RenderHandler* renderHandler)
      : m_renderHandler(renderHandler) {
      ;
   }

   virtual CefRefPtr<CefRenderHandler> GetRenderHandler() {
      return m_renderHandler;
   }

   CefRefPtr<CefRenderHandler> m_renderHandler;

   IMPLEMENT_REFCOUNTING(BrowserClient2);
};
