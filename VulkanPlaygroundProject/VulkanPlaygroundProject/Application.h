#pragma once

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
};

