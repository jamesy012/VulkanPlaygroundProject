#pragma once

class Model;
class Transform;

class RenderManager
{
   //Singleton
private:
   static RenderManager* _RenderManager;
public:
   static inline RenderManager* Get() { return _RenderManager; };

   //Implementation
public:
   void StartUp();

   void AddModel( Model* aModel );
   void AddObject( Model* aModel, Transform* aTransform );
private:
};

