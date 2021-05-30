#pragma once

#include "Pipeline.h"
#include "Buffer.h"
#include "Model.h"

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
   Pipeline mPipelineTest;
   Pipeline mPipeline;
   Model mModelTest;
};

