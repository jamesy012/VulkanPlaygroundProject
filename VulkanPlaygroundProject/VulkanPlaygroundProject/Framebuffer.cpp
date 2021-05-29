#include "stdafx.h"
#include "Framebuffer.h"

#include "RenderPass.h"

bool Framebuffer::Create(VkDevice aDevice, VkExtent2D aSize, RenderPass* aRenderPass, VkImageView& aView) {
   
   VkFramebufferCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   createInfo.renderPass = aRenderPass->GetRenderPass();
   createInfo.width = aSize.width;
   createInfo.height = aSize.height;
   createInfo.attachmentCount = 1;
   createInfo.pAttachments = &aView;
   createInfo.layers = 1;

   VkResult result = vkCreateFramebuffer(aDevice, &createInfo, GetAllocationCallback(), &mFramebuffer);
   ASSERT_VULKAN_SUCCESS(result);
   return true;
}

void Framebuffer::Destroy(VkDevice aDevice) {
}
