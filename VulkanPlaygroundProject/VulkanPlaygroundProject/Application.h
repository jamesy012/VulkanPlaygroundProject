#pragma once

class Window;
class VulkanManager;

class Application {
public:
   void Start();
   void Run();
   void Destroy();
private:
   Window* mWindow;
   VulkanManager* mVkManager;
};

