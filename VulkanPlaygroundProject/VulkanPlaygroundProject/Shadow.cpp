#include "stdafx.h"
#include "Shadow.h"

#include "Image.h"

const VkFormat DEPTHFORMAT = VK_FORMAT_D32_SFLOAT;

ShadowManager* ShadowManager::mInstance = nullptr;

void ShadowManager::Create() {
   ASSERT_IF(mInstance == nullptr);

   mInstance = new ShadowManager();

   mInstance->mRenderPass = new RenderPass();
   mInstance->mRenderPass->Create(_VulkanManager->GetDevice(), VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEPTHFORMAT);
   mInstance->mRenderPass->SetName("Shadow Manager Render Pass");
}

void ShadowManager::Destroy() {
   ASSERT_IF(mInstance != nullptr);
   mInstance->mRenderPass->Destroy(_VulkanManager->GetDevice());
   mInstance->mRenderPass = nullptr;

   delete mInstance;
   mInstance = nullptr;
}

void Shadow::Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout) {
   mSize = aSize;

   mDepthImage = new Image();
   mDepthImage->CreateImage(aSize, DEPTHFORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

   mFramebuffer = new Framebuffer();
   std::vector<VkImageView> depthView = { mDepthImage->GetImageView() };
   mFramebuffer->Create(_VulkanManager->GetDevice(), aSize, ShadowManager::GetRenderPass(), depthView);

   if (aShadowSetLayout != VK_NULL_HANDLE) {
      VkDescriptorSetAllocateInfo setAllocate{};
      setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      setAllocate.descriptorPool = _VulkanManager->GetDescriptorPool();
      setAllocate.descriptorSetCount = 1;
      setAllocate.pSetLayouts = &aShadowSetLayout;
      vkAllocateDescriptorSets(_VulkanManager->GetDevice(), &setAllocate, &mShadowSet);

      UpdateImageDescriptorSet(mDepthImage, mShadowSet, _VulkanManager->GetDefaultSampler(), 1);
   }

   SetName("Shadow");
}

void Shadow::Destroy() {
   mFramebuffer->Destroy(_VulkanManager->GetDevice());
   mDepthImage->Destroy();
   mFramebuffer = nullptr;
   mDepthImage = nullptr;
}

void Shadow::StartRenderPass(VkCommandBuffer aBuffer) {
   _VulkanManager->DebugMarkerStart(aBuffer, "Shadow Render", glm::vec4(0.1f, 0.1f, 0.1f, 0.2f));
   VkClearValue clearColor{};
   clearColor.depthStencil.depth = 1.0f;
   clearColor.depthStencil.stencil = 0;

   VkRenderPassBeginInfo renderBegin{};
   renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderBegin.renderArea.extent = mSize;
   renderBegin.renderPass = ShadowManager::GetRenderPass()->GetRenderPass();//mVkManager->GetPresentRenderPass()->GetRenderPass();
   renderBegin.clearValueCount = 1;
   renderBegin.pClearValues = &clearColor;
   renderBegin.framebuffer = mFramebuffer->GetFramebuffer();//mVkManager->GetPresentFramebuffer(frameIndex)->GetFramebuffer();
   vkCmdBeginRenderPass(aBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
   VkViewport viewport = GetViewportFromExtent2D(mSize);
   VkRect2D scissor = GetRect2DFromExtent2D(mSize);
   vkCmdSetViewport(aBuffer, 0, 1, &viewport);
   vkCmdSetScissor(aBuffer, 0, 1, &scissor);
}

void Shadow::EndRenderPass(VkCommandBuffer aBuffer) {
   vkCmdEndRenderPass(aBuffer);
   _VulkanManager->DebugMarkerEnd(aBuffer);
}

void Shadow::SetName(std::string aName) {
   DebugSetObjName(VK_OBJECT_TYPE_IMAGE, mDepthImage->GetImage(), aName + " Image");
   DebugSetObjName(VK_OBJECT_TYPE_IMAGE_VIEW, mDepthImage->GetImageView(), aName + " Image View");
   mFramebuffer->SetName(aName);
}

void ShadowDirectionalCascade::Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout, uint32_t aNumCascades) {
   mShadows.resize(aNumCascades);
   for (uint32_t i = 0; i < aNumCascades; i++) {
      mShadows[i].Create(aSize, aShadowSetLayout);
   }
   mNumCascades = aNumCascades;
}

void ShadowDirectionalCascade::Destroy() {
   for (uint32_t i = 0; i < mNumCascades; i++) {
      mShadows[i].Destroy();
   }
   mShadows.clear();
   mNumCascades = 0;
}

void ShadowDirectionalCascade::StartRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex) {
   ASSERT_RET(IndexValid(aCascadeIndex));
   mShadows[aCascadeIndex].StartRenderPass(aBuffer);
}

void ShadowDirectionalCascade::EndRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex) {
   ASSERT_RET(IndexValid(aCascadeIndex));
   mShadows[aCascadeIndex].EndRenderPass(aBuffer);
}

bool ShadowDirectionalCascade::IndexValid(uint32_t aCascadeIndex) const {
   return aCascadeIndex < mNumCascades;
}
