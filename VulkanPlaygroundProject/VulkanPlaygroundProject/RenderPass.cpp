#include "stdafx.h"
#include "RenderPass.h"

bool RenderPass::Create(VkDevice aDevice, VkImageLayout aColorLayout, VkFormat aColorFormat) {

   VkAttachmentDescription2 colorDescription{};
   colorDescription.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
   colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
   colorDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   colorDescription.format = aColorFormat;

   VkAttachmentReference2 colorAttachment{};
   colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
   colorAttachment.layout = aColorLayout;
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
   createInfo.attachmentCount = 1;
   createInfo.pAttachments = &colorDescription;

   VkResult result = vkCreateRenderPass2(aDevice, &createInfo, GetAllocationCallback(), &mRenderPass);
   ASSERT_VULKAN_SUCCESS(result);
    return true;
}
