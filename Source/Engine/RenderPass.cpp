#include "stdafx.h"
#include "RenderPass.h"

bool RenderPass::Create(VkDevice aDevice, VkFormat aColorFormat, VkImageLayout aInital, VkImageLayout aFinal, VkFormat aDepthFormat) {
#if WINDOWS
   std::vector<VkAttachmentDescription2> descriptions;
   uint32_t attachmentIndex = 0;

   VkAttachmentReference2 colorAttachment{};
   VkAttachmentReference2 depthAttachment{};

   VkSubpassDescription2 subpass{};
   subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   if (aColorFormat != VK_FORMAT_UNDEFINED) {
      subpass.pColorAttachments = &colorAttachment;
      subpass.colorAttachmentCount = 1;

      colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
      colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachment.attachment = attachmentIndex++;
      colorAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

      VkAttachmentDescription2 colorDescription{};
      colorDescription.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      colorDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorDescription.initialLayout = aInital;
      colorDescription.finalLayout = aFinal;
      colorDescription.format = aColorFormat;
      descriptions.push_back(colorDescription);
   }

   VkRenderPassCreateInfo2 createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
   createInfo.subpassCount = 1;
   createInfo.pSubpasses = &subpass;


   if (aDepthFormat != VK_FORMAT_UNDEFINED) {
      depthAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
      depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depthAttachment.attachment = attachmentIndex++;
      depthAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      VkAttachmentDescription2 depthDescription{};
      depthDescription.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      depthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      if (aColorFormat == VK_FORMAT_UNDEFINED) {
         depthDescription.finalLayout = aFinal;
      } else {
         ASSERT_IF(aFinal == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR || aFinal == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || aFinal == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
         depthDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      }
      depthDescription.format = aDepthFormat;

      subpass.pDepthStencilAttachment = &depthAttachment;
      descriptions.push_back(depthDescription);
   }

   createInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
   createInfo.pAttachments = descriptions.data();

   VkResult result = vkCreateRenderPass2(aDevice, &createInfo, GetAllocationCallback(), &mRenderPass);
#elif APPLE
   std::vector<VkAttachmentDescription> descriptions;
   uint32_t attachmentIndex = 0;

   VkAttachmentReference colorAttachment{};
   VkAttachmentReference depthAttachment{};

   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   if (aColorFormat != VK_FORMAT_UNDEFINED) {
      subpass.pColorAttachments = &colorAttachment;
      subpass.colorAttachmentCount = 1;

      colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachment.attachment = attachmentIndex++;
      //colorAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

      VkAttachmentDescription colorDescription{};
      colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      colorDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorDescription.initialLayout = aInital;
      colorDescription.finalLayout = aFinal;
      colorDescription.format = aColorFormat;
      descriptions.push_back(colorDescription);
   }

   VkRenderPassCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   createInfo.subpassCount = 1;
   createInfo.pSubpasses = &subpass;


   if (aDepthFormat != VK_FORMAT_UNDEFINED) {
      depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depthAttachment.attachment = attachmentIndex++;

      VkAttachmentDescription depthDescription{};
      depthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      if (aColorFormat == VK_FORMAT_UNDEFINED) {
         depthDescription.finalLayout = aFinal;
      } else {
         ASSERT_IF(aFinal == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR || aFinal == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || aFinal == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
         depthDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      }
      depthDescription.format = aDepthFormat;

      subpass.pDepthStencilAttachment = &depthAttachment;
      descriptions.push_back(depthDescription);
   }

   createInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
   createInfo.pAttachments = descriptions.data();
   VkResult result = vkCreateRenderPass(aDevice, &createInfo, GetAllocationCallback(), &mRenderPass);
#endif

   ASSERT_VULKAN_SUCCESS(result);

   mColorFormat = aColorFormat;
   mDepthFormat = aDepthFormat;

   return true;
}

void RenderPass::Destroy(VkDevice aDevice) {
   vkDestroyRenderPass(aDevice, mRenderPass, GetAllocationCallback());
   mRenderPass = VK_NULL_HANDLE;
}

void RenderPass::SetName(std::string aName) {
   DebugSetObjName(VK_OBJECT_TYPE_RENDER_PASS, mRenderPass, aName + " RenderPass");
}
