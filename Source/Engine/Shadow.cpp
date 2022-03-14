#include "stdafx.h"
#include "Shadow.h"

#include "Image.h"

const VkFormat DEPTHFORMAT = VK_FORMAT_D32_SFLOAT;

ShadowManager* ShadowManager::mInstance = nullptr;

void ShadowManager::Create() {
   ASSERT_IF(mInstance == nullptr);

   mInstance = new ShadowManager();

   mInstance->mRenderPass = new RenderPass();
   mInstance->mRenderPass->Create(VulkanManager::Get()->GetDevice(), VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEPTHFORMAT);
   mInstance->mRenderPass->SetName("Shadow Manager Render Pass");
}

void ShadowManager::Destroy() {
   ASSERT_IF(mInstance != nullptr);
   mInstance->mRenderPass->Destroy(VulkanManager::Get()->GetDevice());
   mInstance->mRenderPass = nullptr;

   delete mInstance;
   mInstance = nullptr;
}

void Shadow::Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout, uint32_t aNumCascades) {
   ASSERT_RET(aNumCascades != 0);

   mSize = aSize;
   mNumCascades = aNumCascades;

   mDepthImage = new Image();
   mDepthImage->CreateImage(aSize, DEPTHFORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, aNumCascades);
   mFramebuffers.resize(aNumCascades);
   for (uint32_t i = 0; i < mNumCascades; i++) {
      mFramebuffers[i] = new Framebuffer();
      std::vector<VkImageView> depthView = { mDepthImage->GetArrayImageView(i) };
      mFramebuffers[i]->Create(VulkanManager::Get()->GetDevice(), aSize, ShadowManager::GetRenderPass(), depthView);
   }

   if (aShadowSetLayout != VK_NULL_HANDLE) {
      VkDescriptorSetAllocateInfo setAllocate{};
      setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      setAllocate.descriptorPool = VulkanManager::Get()->GetDescriptorPool();
      setAllocate.descriptorSetCount = 1;
      setAllocate.pSetLayouts = &aShadowSetLayout;
      vkAllocateDescriptorSets(VulkanManager::Get()->GetDevice(), &setAllocate, &mShadowSet);

      ASSERT("change implementation");
      //UpdateImageDescriptorSet(mDepthImage, mShadowSet, VulkanManager::Get()->GetDefaultSampler(), 1);
   }

   SetName("Shadow");
}

void Shadow::Destroy() {
   for (uint32_t i = 0; i < mNumCascades; i++) {
      mFramebuffers[i]->Destroy(VulkanManager::Get()->GetDevice());
   }
   mDepthImage->Destroy();
   mDepthImage = nullptr;
   mFramebuffers.clear();
}

void Shadow::StartRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex) {
   ASSERT_RET(IndexValid(aCascadeIndex));
   VulkanManager::Get()->DebugMarkerStart(aBuffer, "Shadow Render cascade:" + std::to_string(aCascadeIndex), glm::vec4(0.1f, 0.1f, 0.1f, 0.2f));
   VkClearValue clearColor{};
   clearColor.depthStencil.depth = 1.0f;
   clearColor.depthStencil.stencil = 0;

   VkRenderPassBeginInfo renderBegin{};
   renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderBegin.renderArea.extent = mSize;
   renderBegin.renderPass = ShadowManager::GetRenderPass()->GetRenderPass();//mVkManager->GetPresentRenderPass()->GetRenderPass();
   renderBegin.clearValueCount = 1;
   renderBegin.pClearValues = &clearColor;
   renderBegin.framebuffer = mFramebuffers[aCascadeIndex]->GetFramebuffer();//mVkManager->GetPresentFramebuffer(frameIndex)->GetFramebuffer();
   vkCmdBeginRenderPass(aBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
   VkViewport viewport = GetViewportFromExtent2D(mSize);
   VkRect2D scissor = GetRect2DFromExtent2D(mSize);
   vkCmdSetViewport(aBuffer, 0, 1, &viewport);
   vkCmdSetScissor(aBuffer, 0, 1, &scissor);
}

void Shadow::EndRenderPass(VkCommandBuffer aBuffer, uint32_t aCascadeIndex) {
   ASSERT_RET(IndexValid(aCascadeIndex));
   vkCmdEndRenderPass(aBuffer);
   VulkanManager::Get()->DebugMarkerEnd(aBuffer);
}

void Shadow::SetName(std::string aName) {
   DebugSetObjName(VK_OBJECT_TYPE_IMAGE, mDepthImage->GetImage(), aName + " Image");
   DebugSetObjName(VK_OBJECT_TYPE_IMAGE_VIEW, mDepthImage->GetImageView(), aName + " Image View");
   for (uint32_t i = 0; i < mNumCascades; i++) {
      mFramebuffers[i]->SetName(aName + " cascade:" + std::to_string(i));
   }
}

bool Shadow::IndexValid(uint32_t aCascadeIndex) const {
   return aCascadeIndex < mNumCascades;
}
