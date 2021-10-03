#include "stdafx.h"
#include "Pipeline.h"

#include <shaderc/shaderc.hpp>

#include "RenderPass.h"
#include "Vertex.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

shaderc_shader_kind GetShaderCShaderKind( VkShaderStageFlagBits aType ) {
	switch ( aType ) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return shaderc_shader_kind::shaderc_glsl_vertex_shader;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return shaderc_shader_kind::shaderc_glsl_fragment_shader;
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return shaderc_shader_kind::shaderc_glsl_compute_shader;
	default:
		ASSERT( false );
	}
	return shaderc_vertex_shader;
}

VkShaderStageFlagBits GetShaderStageFromFileExt( std::string aFileExt ) {
	if ( aFileExt == "vert" ) {
		return VK_SHADER_STAGE_VERTEX_BIT;
	} else if ( aFileExt == "frag" ) {
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	} else if ( aFileExt == "comp" ) {
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}
	ASSERT( false );
	return (VkShaderStageFlagBits)0;
}

bool Pipeline::AddShader( std::string aPath, bool aForceReload ) {
	LOG_SCOPED_NAME( "Pipeline Shader Compile" );
	LOG( "%s\n", aPath.c_str() );
	Shader shader;

	bool reloadFromFile = false;
	static const bool FORCE_RELOAD = false;

	std::size_t hashValue = stbds_hash_bytes( mShaderMacroArguments.mMacros.data(), sizeof( ShaderMacroArguments::Args ) * mShaderMacroArguments.mMacros.size(), 0 );
	LOG( "hash: %zu\n", hashValue );

	FileIO file( aPath );

	shader.mInfo.stage = GetShaderStageFromFileExt( file.GetFileExtension() );
	shaderc_shader_kind shadercType = GetShaderCShaderKind( shader.mInfo.stage );

	if ( FORCE_RELOAD || aForceReload ) {
		reloadFromFile = true;
	} else {
		reloadFromFile = !file.GetIsHashNewer( hashValue );
	}

	std::vector<uint32_t> spvResult;
	if ( reloadFromFile ) {
		LOG( "Recompiling...\n" );

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		// options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		options.SetOptimizationLevel( shaderc_optimization_level::shaderc_optimization_level_zero );
		options.SetGenerateDebugInfo();
		options.SetWarningsAsErrors();

		for ( size_t i = 0; i < mShaderMacroArguments.mMacros.size(); i++ ) {
			options.AddMacroDefinition( ShaderMacroArguments::ArgsToString( mShaderMacroArguments.mMacros[i] ) );
		}

		// shaderc::PreprocessedSourceCompilationResult result =
		// compiler.PreprocessGlsl(dataStream.str(), shadercType, aPath.c_str(),
		// options);
		//
		// if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		// {
		//   ASSERT_RET_FALSE("Failed shader compilation");
		//}
		// std::string preProcessResult = { result.cbegin(), result.cend() };

		// result = compiler.CompileGlslToSpvAssembly(dataStream.str(), shadercType,
		// aPath.c_str(), options);
		//
		// if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		// {
		//   ASSERT_RET_FALSE("Failed shader compilation");
		//}
		// std::string compileResult = { result.cbegin(), result.cend() };

		std::string fileData = file.Read();
		shaderc::SpvCompilationResult result2 = compiler.CompileGlslToSpv( fileData, shadercType, aPath.c_str(), options );
		if ( result2.GetCompilationStatus() != shaderc_compilation_status_success ) {
			std::cout << result2.GetErrorMessage() << std::endl;
			ASSERT_RET_FALSE( "Failed shader compilation" );
		}
		spvResult = { result2.cbegin(), result2.cend() };

		file.StoreCache( hashValue, (char*)&spvResult[0], static_cast<uint32_t>(spvResult.size() * 4) );
		file.Save();
	} else {
		LOG( "Loading from cache\n" );
		spvResult.resize( file.GetHashFileSize( hashValue ) / 4 );
		file.GetHashFileData( hashValue, (char*)&spvResult[0] );
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spvResult.size() * 4;
	createInfo.pCode = spvResult.data();

	VkResult result = vkCreateShaderModule( VulkanManager::Get()->GetDevice(), &createInfo, GetAllocationCallback(), &shader.mInfo.module );
	ASSERT_VULKAN_SUCCESS_RET_FALSE( result );

	shader.mInfo.pName = "main";

	mShaders.push_back( shader );
	LOG( "Finished\n" );
	return true;
}

void Pipeline::SetVertexType( VertexType& aType ) {
	mVertexType = &aType;
}

void Pipeline::AddDescriptorSetLayout( VkDescriptorSetLayout aSetLayout ) {
	mDescriptorSets.push_back( aSetLayout );
}

void Pipeline::AddPushConstant( VkShaderStageFlags aStage, uint32_t aOffset, uint32_t aSize ) {
	VkPushConstantRange range;
	range.stageFlags = aStage;
	range.offset = aOffset;
	range.size = aSize;
	mPushConstants.push_back( range );
}

bool Pipeline::Create( const VkExtent2D aSize, const RenderPass* aRenderPass ) {
	{
		VkPipelineLayoutCreateInfo pipelineLayout{};
		pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayout.setLayoutCount = static_cast<uint32_t>(mDescriptorSets.size());
		pipelineLayout.pSetLayouts = mDescriptorSets.data();
		pipelineLayout.pushConstantRangeCount = static_cast<uint32_t>(mPushConstants.size());
		pipelineLayout.pPushConstantRanges = mPushConstants.data();
		vkCreatePipelineLayout( VulkanManager::Get()->GetDevice(), &pipelineLayout, nullptr, &mPipelineLayout );
		// helper->setObjName(VK_OBJECT_TYPE_PIPELINE_LAYOUT,
		// (uint64_t)mPipelineLayout, aName + " Layout");
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages( mShaders.size() );
	for ( int i = 0; i < mShaders.size(); i++ ) {
		shaderStages[i] = mShaders[i].mInfo;
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if ( mVertexType == nullptr ) {
		mVertexType = &VertexTypeDefault;
	}

	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(mVertexType->mBindingDescription.size());
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(mVertexType->mAttributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = mVertexType->mBindingDescription.data();
	vertexInputInfo.pVertexAttributeDescriptions = mVertexType->mAttributeDescriptions.data();

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
	rasterizer.cullMode = mCullMode;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_TRUE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f;          // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;          // Optional
	multisampling.pSampleMask = nullptr;            // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE;      // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = mBlending;
	// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	// colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //
	// Optional colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //
	// Optional colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
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
	depthStencil.back = {};  // Optional

	mDynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT );
	mDynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR );

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(mDynamicStates.size());
	dynamicState.pDynamicStates = mDynamicStates.data();

	bool canRender = true;
	// todo make rasteraization check better
	for ( size_t i = 0; i < mShaders.size(); i++ ) {
		if ( mShaders[i].mInfo.stage == VK_SHADER_STAGE_COMPUTE_BIT ) {
			canRender = false;
		}
	}

	if ( canRender ) {
		VkGraphicsPipelineCreateInfo graphicsPipeline{};
		graphicsPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipeline.layout = mPipelineLayout;
		graphicsPipeline.stageCount = static_cast<uint32_t>(shaderStages.size());
		graphicsPipeline.pStages = shaderStages.data();
		graphicsPipeline.pViewportState = &viewportState;
		graphicsPipeline.pVertexInputState = &vertexInputInfo;
		graphicsPipeline.pInputAssemblyState = &inputAssembly;
		graphicsPipeline.pRasterizationState = &rasterizer;
		graphicsPipeline.pMultisampleState = &multisampling;
		graphicsPipeline.pDepthStencilState = &depthStencil;
		graphicsPipeline.pColorBlendState = &colorBlending;
		graphicsPipeline.pDynamicState = &dynamicState;
		graphicsPipeline.renderPass = aRenderPass->GetRenderPass();
		graphicsPipeline.subpass = 0;
		graphicsPipeline.basePipelineHandle = VK_NULL_HANDLE; // Optional
		graphicsPipeline.basePipelineIndex = -1;              // Optional
		vkCreateGraphicsPipelines( VulkanManager::Get()->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipeline, nullptr, &mPipeline );
		// helper->setObjName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)mPipeline, aName);
	} else {
		if ( shaderStages.size() == 1 ) {
			VkComputePipelineCreateInfo computePipeline{};
			computePipeline.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipeline.layout = mPipelineLayout;
			computePipeline.stage = shaderStages[0];

			vkCreateComputePipelines( VulkanManager::Get()->GetDevice(), VK_NULL_HANDLE, 1, &computePipeline, nullptr, &mPipeline );
		} else {
			ASSERT( "Number of Shader Stages incorrect" );
		}
	}

	for ( size_t i = 0; i < mShaders.size(); i++ ) {
		vkDestroyShaderModule( VulkanManager::Get()->GetDevice(), mShaders[i].mInfo.module, GetAllocationCallback() );
	}
	mShaders.clear();

	return false;
}

void Pipeline::Destroy() {
	if ( mPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( VulkanManager::Get()->GetDevice(), mPipeline, nullptr );
	}
	if ( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( VulkanManager::Get()->GetDevice(), mPipelineLayout, nullptr );
	}
	mPipeline = VK_NULL_HANDLE;
	mPipelineLayout = VK_NULL_HANDLE;
}
