#pragma once

class Model;
class Transform;
class BufferStorageUniform;
class Material;

struct RenderingBufferOUT;
class BufferVertex;
class BufferIndex;

struct RenderCommand {
	Model* mModel;
	Transform* mTransform;
	Material* mMaterial;
};

class RenderManager {
	//Singleton
private:
	static RenderManager* _RenderManager;
public:
	static inline RenderManager* Get() { return _RenderManager; };

	//Implementation
public:
	RenderManager();
	~RenderManager();

	void StartUp();

	void AddModel(Model* aModel);
	void AddObject(Model* aModel, Transform* aTransform);

	void Render(VkCommandBuffer aBuffer);
private:


};

