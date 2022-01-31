#include "stdafx.h"
#include "RenderManager.h"

#include "Model.h"
#include "Buffer.h"
#include "Vertex.h"

RenderManager* RenderManager::_RenderManager = nullptr;

RenderManager::RenderManager() {
	ASSERT_IF(_RenderManager == nullptr);
	_RenderManager = this;
}

RenderManager::~RenderManager() {
	_RenderManager = nullptr;
}

void RenderManager::StartUp() {

}

void RenderManager::AddModel(Model* aModel) {

}

void RenderManager::AddObject(Model* aModel, Transform* aTransform) {
}

void RenderManager::Render(VkCommandBuffer aBuffer) {
}

