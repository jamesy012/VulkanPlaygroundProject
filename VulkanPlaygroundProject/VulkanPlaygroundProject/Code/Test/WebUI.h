#pragma once

#if WEB_UI_ENABLED == 1
#include <AppCore/JSHelpers.h>
#endif

class WebUI {
public:
	void SetupMaterials(std::vector<VkDescriptorSetLayout> aSetLayouts, const RenderPass* aRenderPass);
	void Start();
	void Update();
	void Render(VkCommandBuffer aCommandBuffer, class BufferVertex* aScreenSpaceQuad);
	void Destroy();

	VkDescriptorSet& GetDescriptorSet();

private:

#if WEB_UI_ENABLED == 1
	void JavaScriptTest(const ultralight::JSObject& thisObject, const ultralight::JSArgs& args);
	ultralight::JSValue JavaScriptTestRet(const ultralight::JSObject& thisObject, const ultralight::JSArgs& args);
#endif
};