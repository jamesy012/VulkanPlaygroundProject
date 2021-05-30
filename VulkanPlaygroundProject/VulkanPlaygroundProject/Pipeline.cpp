#include "stdafx.h"
#include "Pipeline.h"

#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>

#include "RenderPass.h"
#include "Vertex.h"

shaderc_shader_kind GetShaderCShaderKind(VkShaderStageFlagBits aType) {
   switch (aType) {
      case VK_SHADER_STAGE_VERTEX_BIT:
         return shaderc_shader_kind::shaderc_glsl_vertex_shader;
      case VK_SHADER_STAGE_FRAGMENT_BIT:
         return shaderc_shader_kind::shaderc_glsl_fragment_shader;
      default:
         assert(false);
   }
   return shaderc_vertex_shader;
}

bool Pipeline::AddShader(std::string aPath) {
   Shader shader;

   std::string fileExt;
   {
      size_t index = aPath.find_last_of('.') + 1;
      if (index == 0) {
         ASSERT_RET_FALSE("Invalid file name?");
      }
      fileExt = aPath.substr(index);
      for (auto& c : fileExt) {
         c = std::tolower(c);
      }
   }
   if (fileExt == "vert") {
      shader.mInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
   } else if (fileExt == "frag") {
      shader.mInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   }
   shaderc_shader_kind shadercType = GetShaderCShaderKind(shader.mInfo.stage);

   std::ifstream fileStream(aPath);
   std::stringstream dataStream;
   dataStream << fileStream.rdbuf();

   shaderc::Compiler compiler;
   shaderc::CompileOptions options;

   //shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(dataStream.str(), shadercType, aPath.c_str(), options);
   //
   //if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
   //   ASSERT_RET_FALSE("Failed shader compilation");
   //}
   //std::string preProcessResult = { result.cbegin(), result.cend() };
   //std::cout << result.GetErrorMessage() << std::endl;

   //result = compiler.CompileGlslToSpvAssembly(dataStream.str(), shadercType, aPath.c_str(), options);
   //
   //if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
   //   ASSERT_RET_FALSE("Failed shader compilation");
   //}
   //std::string compileResult = { result.cbegin(), result.cend() };

   std::string fileData = dataStream.str();
   shaderc::SpvCompilationResult result2 = compiler.CompileGlslToSpv(fileData, shadercType, aPath.c_str(), options);
   if (result2.GetCompilationStatus() != shaderc_compilation_status_success) {
      std::cout << result2.GetErrorMessage() << std::endl;
      ASSERT_RET_FALSE("Failed shader compilation");
   }
   std::vector<uint32_t> spvResult = { result2.cbegin(), result2.cend() };
   std::cout << result2.GetErrorMessage() << std::endl;

   VkShaderModuleCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = spvResult.size() * 4;
   createInfo.pCode = spvResult.data();


   VkResult result = vkCreateShaderModule(_VulkanManager->GetDevice(), &createInfo, GetAllocationCallback(), &shader.mInfo.module);
   ASSERT_VULKAN_SUCCESS_RET_FALSE(result);

   shader.mInfo.pName = "main";

   mShaders.push_back(shader);
   return true;
}

bool Pipeline::Create(const VkExtent2D aSize, const RenderPass* aRenderPass) {

   {
      VkPipelineLayoutCreateInfo pipelineLayout{};
      pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      //pipelineLayout.setLayoutCount = mDescriptorSets.size();
      //pipelineLayout.pSetLayouts = mDescriptorSets.data(); // Optional
      //pipelineLayout.pushConstantRangeCount = mPushConstants.size(); // Optional
      //pipelineLayout.pPushConstantRanges = mPushConstants.data(); // Optional
      vkCreatePipelineLayout(_VulkanManager->GetDevice(), &pipelineLayout, nullptr, &mPipelineLayout);
      //helper->setObjName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)mPipelineLayout, aName + " Layout");
   }

   std::vector <VkPipelineShaderStageCreateInfo> shaderStages(mShaders.size());
   for (int i = 0; i < mShaders.size(); i++) {
      shaderStages[i] = mShaders[i].mInfo;
   }

   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   //if (mVertexType == nullptr) {
   VertexType* vertexType = &VertexTypeSimple;
   //}
   auto bindingDescription = vertexType->mBindingDescription;
   auto attributeDescriptions = vertexType->mAttributeDescriptions;
   vertexInputInfo.vertexBindingDescriptionCount = 1;
   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
   vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
   vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

   VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
   inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = (float)aSize.width;
   viewport.height = (float)aSize.height;
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor{};
   scissor.offset = { 0, 0 };
   scissor.extent = aSize;

   VkPipelineViewportStateCreateInfo viewportState{};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   VkPipelineRasterizationStateCreateInfo rasterizer{};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
   rasterizer.lineWidth = 1.0f;
   rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;// mCullMode;
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   rasterizer.depthBiasEnable = VK_TRUE;
   rasterizer.depthBiasConstantFactor = 0.0f; // Optional
   rasterizer.depthBiasClamp = 0.0f; // Optional
   rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

   VkPipelineMultisampleStateCreateInfo multisampling{};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisampling.minSampleShading = 1.0f; // Optional
   multisampling.pSampleMask = nullptr; // Optional
   multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
   multisampling.alphaToOneEnable = VK_FALSE; // Optional

   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
   colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_FALSE;
   //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
   //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
   //colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
   //colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
   //colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
   //colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo colorBlending{};
   colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;
   colorBlending.logicOp = VK_LOGIC_OP_CLEAR; // Optional
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;
   colorBlending.blendConstants[0] = 0.0f; // Optional
   colorBlending.blendConstants[1] = 0.0f; // Optional
   colorBlending.blendConstants[2] = 0.0f; // Optional
   colorBlending.blendConstants[3] = 0.0f; // Optional

   VkPipelineDepthStencilStateCreateInfo depthStencil{};
   depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depthStencil.depthTestEnable = VK_TRUE;
   depthStencil.depthWriteEnable = VK_TRUE;
   depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
   depthStencil.depthBoundsTestEnable = VK_FALSE;
   depthStencil.minDepthBounds = 0.0f; // Optional
   depthStencil.maxDepthBounds = 0.0f; // Optional
   depthStencil.stencilTestEnable = VK_FALSE;
   depthStencil.front = {}; // Optional
   depthStencil.back = {}; // Optional

   VkPipelineDynamicStateCreateInfo dynamicState{};
   //dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   //dynamicState.dynamicStateCount = mDynamicStates.size();
   //dynamicState.pDynamicStates = mDynamicStates.data();

   VkGraphicsPipelineCreateInfo pipeline{};
   pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipeline.layout = mPipelineLayout;
   pipeline.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipeline.pStages = shaderStages.data();
   pipeline.pVertexInputState = &vertexInputInfo;
   pipeline.pInputAssemblyState = &inputAssembly;
   pipeline.pViewportState = &viewportState;
   pipeline.pRasterizationState = &rasterizer;
   pipeline.pMultisampleState = &multisampling;
   pipeline.pDepthStencilState = &depthStencil;
   pipeline.pColorBlendState = &colorBlending;
   //pipeline.pDynamicState = &dynamicState;
   pipeline.renderPass = aRenderPass->GetRenderPass();
   pipeline.subpass = 0;
   pipeline.basePipelineHandle = VK_NULL_HANDLE; // Optional
   pipeline.basePipelineIndex = -1; // Optional
   vkCreateGraphicsPipelines(_VulkanManager->GetDevice(), VK_NULL_HANDLE, 1, &pipeline, nullptr, &mPipeline);
   //helper->setObjName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)mPipeline, aName);

   for (size_t i = 0; i < mShaders.size(); i++) {
      vkDestroyShaderModule(_VulkanManager->GetDevice(), mShaders[i].mInfo.module, GetAllocationCallback());
   }
   mShaders.clear();

   return false;
}

void Pipeline::Destroy() {
   if (mPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(_VulkanManager->GetDevice(), mPipeline, nullptr);
   }
   if (mPipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(_VulkanManager->GetDevice(), mPipelineLayout, nullptr);
   }
   mPipeline = VK_NULL_HANDLE;
   mPipelineLayout = VK_NULL_HANDLE;
}
