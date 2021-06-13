#include "stdafx.h"
#include "RenderPass.h"

bool RenderPass::Create(VkDevice aDevice, VkFormat aColorFormat, VkImageLayout aInital, VkImageLayout aFinal, VkFormat aDepthFormat) {

   VkAttachmentDescription2 colorDescription{};
   colorDescription.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
   colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
   colorDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorDescription.initialLayout = aInital;
   colorDescription.finalLayout = aFinal;
   colorDescription.format = aColorFormat;

   VkAttachmentReference2 colorAttachment{};
   colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
   colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   colorAttachment.attachment = 0;
   colorAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

   VkSubpassDescription2 subpass{};
   subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.pColorAttachments = &colorAttachment;
   subpass.colorAttachmentCount = 1;

   VkRenderPassCreateInfo2 createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
   createInfo.subpassCount = 1;
   createInfo.pSubpasses = &subpass;


   VkAttachmentReference2 depthAttachment{};
   VkAttachmentDescription2 depthDescription{};
   if (aDepthFormat != VK_FORMAT_UNDEFINED) {
      depthAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
      depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depthAttachment.attachment = 1;
      depthAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      depthDescription.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      depthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      ASSERT_IF(aFinal == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR || aFinal == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || aFinal == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
      depthDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depthDescription.format = aDepthFormat;

      subpass.pDepthStencilAttachment = &depthAttachment;
      VkAttachmentDescription2 attachments[] = { colorDescription, depthDescription };
      createInfo.attachmentCount = 2;
      createInfo.pAttachments = attachments;
   } else {
      createInfo.attachmentCount = 1;
      createInfo.pAttachments = &colorDescription;
   }

   VkResult result = vkCreateRenderPass2(aDevice, &createInfo, GetAllocationCallback(), &mRenderPass);
   ASSERT_VULKAN_SUCCESS(result);

   mColorFormat = aColorFormat;
   mDepthFormat = aDepthFormat;

   return true;
}

void RenderPass::Destroy(VkDevice aDevice) {
   vkDestroyRenderPass(aDevice, mRenderPass, GetAllocationCallback());
   mRenderPass = VK_NULL_HANDLE;
}
