#pragma once

#include <include/cef_render_handler.h>
#include <include/cef_client.h>
#include <include/cef_browser.h>


int BrowserStart(struct GLFWwindow* aWindow, bool quickTest = false);
void BrowserUpdate();
void BrowserDestroy();

class RenderHandler : public CefRenderHandler {
public:

   RenderHandler() {
   }

   // FrameListener interface
public:
   bool frameStarted() {

      //m_renderNode->yaw(Ogre::Radian(evt.timeSinceLastFrame) * Ogre::Math::PI * 2. * (1. / 10.)); // one turn in 10sec

      //CefDoMessageLoopWork();

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
