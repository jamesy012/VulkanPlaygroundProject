#pragma once

#include "Pipeline.h"
#include "Buffer.h"

class Window;
class VulkanManager;

class Application {
public:
   void Start();
   void Run();
   void Destroy();
private:
   void Draw();

   Window* mWindow;
   VulkanManager* mVkManager;

private:
   BufferVertex mScreenQuad;
   Pipeline mPipeline;

};

