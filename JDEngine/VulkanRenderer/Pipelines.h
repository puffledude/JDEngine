#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/mat4x4.hpp>
#include <fstream>
#include <iostream>
#include "Vertex.h"


namespace JD {

	static std::vector<char> readFile(const std::string& filename) {
		std::cout << "Reading shader file: " << filename << std::endl;
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}

		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
		return buffer;
	}


	[[nodiscard]] vk::ShaderModule createShaderModule(const std::vector<char>& code, vk::Device device)
	{
		vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
		vk::ShaderModule shaderModule;
		device.createShaderModule(&createInfo, nullptr, &shaderModule);
		return shaderModule;
	}
	

	void createSkyboxPipelinefunc(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout, vk::Device device, vk::DescriptorSetLayout descriptorSetLayout, float width, float height, vk::Format swapChainFormat){
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/skybox.slang.spv"), device);
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = 1, //Apparently optimised out the other two unused variables
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		.topology = vk::PrimitiveTopology::eTriangleList,
		};

		vk::Viewport viewport{ 0.0f, 0.0f, width, height, 0.0f, 1.0f };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = VK_FALSE };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout };
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = swapChainFormat;

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		{.stageCount = 2,
		 .pStages = shaderStages,
		 .pVertexInputState = &vertexInputInfo,
		 .pInputAssemblyState = &inputAssembly,
		 .pViewportState = &viewportState,
		 .pRasterizationState = &rasterizer,
		 .pMultisampleState = &multisampling,
		 .pColorBlendState = &colorBlending,
		 .pDynamicState = &dynamicState,
		 .layout = pipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat} };

		auto pipelineResult = device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create skybox pipeline!");
		}
		pipeline = pipelineResult.value;
		device.destroyShaderModule(shaderModule);
	}


	void createShadowPipelinefunc(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout, vk::Device device, vk::DescriptorSetLayout& descriptorSetLayout, float width, float height, vk::Format swapChainFormat, vk::Format depthFormat) {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/directionalShadow.slang.spv"), device);
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		.topology = vk::PrimitiveTopology::eTriangleList,
		};
		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(width),
			static_cast<float>(height), 0.0f, 1.0f };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
		};
		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = VK_FALSE };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment
		};
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
		};

		vk::PushConstantRange pushConstantRange{
		.stageFlags = vk::ShaderStageFlagBits::eVertex,
		.offset = 0,
		.size = sizeof(uint32_t)
		};


		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange };
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(swapChainFormat);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		{.stageCount = 2,
		 .pStages = shaderStages,
		 .pVertexInputState = &vertexInputInfo,
		 .pInputAssemblyState = &inputAssembly,
		 .pViewportState = &viewportState,
		 .pRasterizationState = &rasterizer,
		 .pMultisampleState = &multisampling,
		 .pDepthStencilState = &depthStencil,
		 .pColorBlendState = &colorBlending,
		 .pDynamicState = &dynamicState,
		 .layout = pipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat, .depthAttachmentFormat = depthFormat} };

		auto pipelineResult = device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create skybox pipeline!");
		}
		pipeline = pipelineResult.value;
		device.destroyShaderModule(shaderModule);


	}

	void createGBufferPipelinefunc(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout, vk::Device device, vk::DescriptorSetLayout& descriptorSetLayout, float width, float height, vk::Format swapChainFormat, vk::Format depthFormat) {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/gbuffer.slang.spv"), device);
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		.topology = vk::PrimitiveTopology::eTriangleList,
		};
		vk::Viewport viewport{ 0.0f, 0.0f, width,
			height, 0.0f, 1.0f };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = VK_FALSE };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		std::array blendAttachments = { colorBlendAttachment, colorBlendAttachment,  colorBlendAttachment, colorBlendAttachment, colorBlendAttachment };

		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = static_cast<uint32_t>(blendAttachments.size()), .pAttachments = blendAttachments.data() };

		// --- Push constant configuration added ---
		vk::PushConstantRange pushConstantRange{
			.stageFlags = vk::ShaderStageFlagBits::eVertex,
			.offset = 0,
			.size = sizeof(GbufferPushConstants)
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange };
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
		};
		const vk::Format gbufferColourFormat = vk::Format::eB8G8R8A8Srgb;
		const vk::Format gbufferNormalFormat = vk::Format::eB8G8R8A8Unorm;
		const vk::Format gbufferMaterialFormat = vk::Format::eB8G8R8A8Unorm;
		const vk::Format positionFormat = vk::Format::eR16G16B16A16Sfloat;
		const vk::Format velocityFormat = vk::Format::eR16G16B16A16Sfloat;
		std::array<vk::Format, 5> colorAttachmentFormats = { gbufferColourFormat, gbufferNormalFormat, gbufferMaterialFormat, positionFormat, velocityFormat };
		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		{.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = pipelineLayout,
		.renderPass = nullptr},
		{.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()), .pColorAttachmentFormats = colorAttachmentFormats.data(), .depthAttachmentFormat = depthFormat} };

		auto pipelineResult = device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create gbuffer pipeline!");
		}
		pipeline = pipelineResult.value;
		device.destroyShaderModule(shaderModule);
	}

	void createLightingPipelinefunc(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout, vk::Device device, vk::DescriptorSetLayout& descriptorSetLayout, float width, float height, vk::Format swapChainFormat) {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/lighting.slang.spv"), device);
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()), //Apparently optimised out the other two unused variables
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		.topology = vk::PrimitiveTopology::eTriangleList,
		};

		vk::Viewport viewport{ 0.0f, 0.0f, width,
		height, 0.0f, 1.0f };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = VK_FALSE };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment
		};

		vk::PushConstantRange pushConstantRange{
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
		.offset = 0,
		.size = sizeof(uint32_t)
		};

		//Push constant number of lights
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 1,.pPushConstantRanges = &pushConstantRange };

		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(swapChainFormat);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		{.stageCount = 2,
		 .pStages = shaderStages,
		 .pVertexInputState = &vertexInputInfo,
		 .pInputAssemblyState = &inputAssembly,
		 .pViewportState = &viewportState,
		 .pRasterizationState = &rasterizer,
		 .pMultisampleState = &multisampling,
		 .pColorBlendState = &colorBlending,
		 .pDynamicState = &dynamicState,
		 .layout = pipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat} };

		auto pipelineResult = device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create final output pipeline!");
		}
		pipeline= pipelineResult.value;
		device.destroyShaderModule(shaderModule);
	}


	void createTaaPipelinefunc(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout, vk::Device device, vk::DescriptorSetLayout& descriptorSetLayout, float width, float height, vk::Format swapChainFormat){
	
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/TAA.slang.spv"), device);
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		.topology = vk::PrimitiveTopology::eTriangleList,
		};

		vk::Viewport viewport{ 0.0f, 0.0f, width,
		height, 0.0f, 1.0f };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = VK_FALSE };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout };
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(swapChainFormat);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		{.stageCount = 2,
		 .pStages = shaderStages,
		 .pVertexInputState = &vertexInputInfo,
		 .pInputAssemblyState = &inputAssembly,
		 .pViewportState = &viewportState,
		 .pRasterizationState = &rasterizer,
		 .pMultisampleState = &multisampling,
		 .pColorBlendState = &colorBlending,
		 .pDynamicState = &dynamicState,
		 .layout = pipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat} };

		auto pipelineResult = device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create final output pipeline!");
		}
		pipeline = pipelineResult.value;
		device.destroyShaderModule(shaderModule);
	}






	void createFinalOutputPipelinefunc(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout, vk::Device device, vk::DescriptorSetLayout& descriptorSetLayout, float width, float height, vk::Format swapChainFormat) {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/finalOut.slang.spv"), device);
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()), //Apparently optimised out the other two unused variables
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		.topology = vk::PrimitiveTopology::eTriangleList,
		};

		vk::Viewport viewport{ 0.0f, 0.0f, width,
		height, 0.0f, 1.0f };

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = VK_FALSE };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout };
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(swapChainFormat);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		{.stageCount = 2,
		 .pStages = shaderStages,
		 .pVertexInputState = &vertexInputInfo,
		 .pInputAssemblyState = &inputAssembly,
		 .pViewportState = &viewportState,
		 .pRasterizationState = &rasterizer,
		 .pMultisampleState = &multisampling,
		 .pColorBlendState = &colorBlending,
		 .pDynamicState = &dynamicState,
		 .layout = pipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat} };

		auto pipelineResult = device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create final output pipeline!");
		}
		pipeline = pipelineResult.value;
		device.destroyShaderModule(shaderModule);

	}
};