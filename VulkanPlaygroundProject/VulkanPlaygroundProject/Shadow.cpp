#include "stdafx.h"
#include "Shadow.h"

#include "Image.h"

const VkFormat DEPTHFORMAT = VK_FORMAT_D32_SFLOAT;

void Shadow::Create(VkExtent2D aSize, VkDescriptorSetLayout aShadowSetLayout) {
   mSize = aSize;

   mDepthImage = new Image();
   mDepthImage->CreateImage(aSize, DEPTHFORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

   mRenderPass = new RenderPass();
   mRenderPass->Create(_VulkanManager->GetDevice(), VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEPTHFORMAT);

   mFramebuffer = new Framebuffer();
   std::vector<VkImageView> depthView = { mDepthImage->GetImageView() };
   mFramebuffer->Create(_VulkanManager->GetDevice(), aSize, mRenderPass, depthView);

   if (aShadowSetLayout != VK_NULL_HANDLE) {
      VkDescriptorSetAllocateInfo setAllocate{};
      setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      setAllocate.descriptorPool = _VulkanManager->GetDescriptorPool();
      setAllocate.descriptorSetCount = 1;
      setAllocate.pSetLayouts = &aShadowSetLayout;
      vkAllocateDescriptorSets(_VulkanManager->GetDevice(), &setAllocate, &mShadowSet);

      UpdateImageDescriptorSet(mDepthImage, mShadowSet, _VulkanManager->GetDefaultSampler(), 1);
   }
}

void Shadow::Destroy() {
   mFramebuffer->Destroy(_VulkanManager->GetDevice());
   mRenderPass->Destroy(_VulkanManager->GetDevice());
   mDepthImage->Destroy();
   mFramebuffer = nullptr;
   mRenderPass = nullptr;
   mDepthImage = nullptr;
}

void Shadow::StartRenderPass(VkCommandBuffer aBuffer) {
   VkClearValue clearColor{};
   clearColor.depthStencil.depth = 1.0f;
   clearColor.depthStencil.stencil = 0;

   VkRenderPassBeginInfo renderBegin{};
   renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderBegin.renderArea.extent = mSize;
   renderBegin.renderPass = mRenderPass->GetRenderPass();//mVkManager->GetPresentRenderPass()->GetRenderPass();
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
}
