#include "VulkanRenderer.h"
//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#define VMA_IMPLEMENTATION
// Now include the headers so they catch the defines above!
#include "tiny_gltf_v3.h"
#include "vma/vk_mem_alloc.h"

// Then your project headers

#include "RequiredFeatures.h"

namespace JD
{
	VulkanRenderer::VulkanRenderer(Gameworld& world) :Renderer(world) {
	}

	VulkanRenderer::~VulkanRenderer() {
		cleanupVulkan();
	}

	void VulkanRenderer::recreateSwapChain() {
		vulkanCore.device.waitIdle();
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		cleanupSwapChain();
		createSwapChain();
		createImageViews();
		createDepthResources();

	}


	void VulkanRenderer::cleanupVulkan() {
		// 1. Wait for everything on the GPU to finish before destroying anything
		/*if (vulkanCore.device.device != VK_NULL_HANDLE) {
			vkDeviceWaitIdle(vulkanCore.device.device);
		}*/
		vulkanCore.device.waitIdle();

		// 2. Clear out the swap chain and other abstractions
		cleanupSwapChain();
	};

	void VulkanRenderer::cleanupSwapChain() {
		vulkanCore.swapChainImages.clear();
	}

	void VulkanRenderer::AssignWindow(GLFWwindow* window) {
		this->window = window;
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		initVulkan();
	}
	void VulkanRenderer::initVulkan() {
		try {
			vkb::InstanceBuilder builder;
			auto inst_ret = builder.set_app_name("JDEngine").request_validation_layers().require_api_version(1,3)
				.use_default_debug_messenger().build();
			if (!inst_ret) {
				throw std::runtime_error("Failed to create Vulkan instance: " + inst_ret.error().message());
			}
			vulkanCore.instance = inst_ret.value();
			
			// Initialize dispatcher with vkGetInstanceProcAddr globally
			VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanCore.instance.fp_vkGetInstanceProcAddr);
			// Initialize dispatcher with instance level functions
			VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanCore.instance.instance, vulkanCore.instance.fp_vkGetInstanceProcAddr);

			vulkanCore.surface = create_surface_glfw(vulkanCore.instance, window);
			createDevices();
			createSwapChain();
			initVMA();
			createCommandPool();
			createQueues();
			createImageViews();
			createDescriptorSetLayouts();
			createDescriptorPool();
			createDepthResources();

			createGraphicsPipelines();

			createCommandPool();

			createDescriptorSets();
			createCommandBuffers();
			createSyncObjects();
			
			std::cout << "Vulkan initialized successfully!" << std::endl;

		}
		catch (const std::exception& e) {
			std::cerr << "Failed to initialize Vulkan: " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	void VulkanRenderer::createDevices() {
		vkb::PhysicalDeviceSelector physDeviceSelector(vulkanCore.instance);
		auto phys_device_ret = physDeviceSelector.set_surface(vulkanCore.surface).set_minimum_version(1, 3)
			.set_required_features_11(JD::RequiredPhysicalFeatures().vulkan11Features)
			.set_required_features_13(JD::RequiredPhysicalFeatures().vulkan13Features)
			.add_required_extension_features(JD::RequiredPhysicalFeatures().extendedDynamicStateFeatures)
			.select();
		if (!phys_device_ret) {
			std::cout << phys_device_ret.error().message() << "\n";
			if (phys_device_ret.error() == vkb::PhysicalDeviceError::no_suitable_device) {
				const auto& detailed_reasons = phys_device_ret.detailed_failure_reasons();
				if (!detailed_reasons.empty()) {
					std::cerr << "GPU Selection failure reasons:\n";
					for (const std::string& reason : detailed_reasons) {
						std::cerr << reason << "\n";
					}
				}
			}
			throw std::runtime_error("Failed to select physical device: " + phys_device_ret.error().message());
		}
		vkb::PhysicalDevice physDevice = phys_device_ret.value();
		vkb::DeviceBuilder deviceBuilder(physDevice);
		auto device_ret = deviceBuilder.build();
		if (!device_ret) {
			std::cout << device_ret.error().message() << "\n";
			return;
		}
		vulkanCore.vkbInstances.device = device_ret.value();
		vulkanCore.device = vulkanCore.vkbInstances.device;

		// 3. Load device-level extension functions into vulkan.hpp
		VULKAN_HPP_DEFAULT_DISPATCHER.init(
			vulkanCore.instance.instance,
			vulkanCore.instance.fp_vkGetInstanceProcAddr,
			vulkanCore.vkbInstances.device.device,
			vulkanCore.vkbInstances.device.fp_vkGetDeviceProcAddr
		);
	}

	void VulkanRenderer::createSwapChain() {

		vkb::SwapchainBuilder swapchainBuilder(vulkanCore.vkbInstances.device);

		VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

		auto swap_ret = swapchainBuilder.set_old_swapchain(vulkanCore.swapChain). set_desired_format(desiredFormat)
			.add_fallback_format({ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			 .use_default_present_mode_selection()
			 .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			.build();
		if (!swap_ret) {
			std::cout << swap_ret.error().message() << " " << swap_ret.vk_result() << "\n";
			return;
		}
		vkb::destroy_swapchain(vulkanCore.vkbInstances.swapChain);
		vulkanCore.vkbInstances.swapChain = swap_ret.value();
		vulkanCore.swapChain = vulkanCore.vkbInstances.swapChain;
		vulkanCore.swapChainImages = vulkanCore.device.getSwapchainImagesKHR(vulkanCore.swapChain);
	}

	void VulkanRenderer::initVMA() {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = vulkanCore.vkbInstances.device.physical_device;
		allocatorInfo.device = vulkanCore.device;
		allocatorInfo.instance = vulkanCore.instance.instance;
		if (vmaCreateAllocator(&allocatorInfo, &vulkanCore.allocator) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VMA allocator");
		}
	}

	vk::ImageView VulkanRenderer::createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
		vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
			.format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
		viewInfo.subresourceRange.levelCount = mipLevels;
		return vulkanCore.device.createImageView(viewInfo);		//return vk::ImageView(static_cast<vk::Device>(vulkanCore.device.device), viewInfo);
	}


	void VulkanRenderer::createImageViews() {
		assert(vulkanCore.swapChainImageViews.empty());
		vulkanCore.swapChainImageViews.reserve(vulkanCore.swapChainImages.size());

		for (uint32_t i = 0; i < vulkanCore.swapChainImages.size(); i++) {
			vulkanCore.swapChainImageViews.push_back(createImageView(vulkanCore.swapChainImages[i], static_cast<vk::Format>(vulkanCore.vkbInstances.swapChain.image_format), vk::ImageAspectFlagBits::eColor, 1));
		}

	}

	void VulkanRenderer::copyBuffer(const vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size) {
		vk::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
		commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
		endSingleTimeCommands(commandCopyBuffer);
	}

	uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		//vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
		vk::PhysicalDevice physDevice = vulkanCore.vkbInstances.device.physical_device.physical_device;
		vk::PhysicalDeviceMemoryProperties memProperties = physDevice.getMemoryProperties();
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("Failed to find suitable memory type!");
	}

	void VulkanRenderer::createVertexBuffer(std::vector<Vertex>& verticies, vk::Buffer& buffer) {
		vk::DeviceSize bufferSize = sizeof(verticies[0]) * verticies.size();
		vk::Buffer stagingBuffer = vk::Buffer{};
		VmaAllocation stagingBufferMemory = VmaAllocation{};
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
		void* mapped = nullptr;
		vmaMapMemory(vulkanCore.allocator, stagingBufferMemory, &mapped);
		std::memcpy(mapped, verticies.data(), (size_t)bufferSize);
		vmaUnmapMemory(vulkanCore.allocator, stagingBufferMemory);
		VmaAllocation vertexBufferMemory = VmaAllocation{};
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, buffer, bufferSize);


	}

	void VulkanRenderer::createIndexBuffer(std::vector<uint32_t>& indicies, vk::Buffer& buffer) {
		vk::DeviceSize bufferSize = sizeof(indicies[0]) * indicies.size();
		vk::Buffer stagingBuffer = vk::Buffer{};
		VmaAllocation stagingBufferMemory = VmaAllocation{};
		VmaAllocation indexBufferMemory = VmaAllocation{};
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
		void* mapped = nullptr;
		vmaMapMemory(vulkanCore.allocator, stagingBufferMemory, &mapped);
		std::memcpy(mapped, indicies.data(), (size_t)bufferSize);
		vmaUnmapMemory(vulkanCore.allocator, stagingBufferMemory);
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, indexBufferMemory);
		copyBuffer(stagingBuffer, buffer, bufferSize);
		
	}

	void VulkanRenderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, VmaAllocation& allocation){
		vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
		const VkBufferCreateInfo vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		// Map the vulkan.hpp memory properties to VMA's required flags
		allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(properties);

		// If we are requesting host visible memory (for staging buffers), tell VMA we need sequential write access
		if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}

		VkBuffer cBuffer = VK_NULL_HANDLE;
		VmaAllocation createdAllocation = VK_NULL_HANDLE;

		if (vmaCreateBuffer(vulkanCore.allocator, &vkBufferInfo, &allocInfo, &cBuffer, &createdAllocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VMA buffer!");
		}

		buffer = vk::Buffer(cBuffer);
		allocation = createdAllocation;
	}

	void VulkanRenderer::transitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) {
		auto commandBuffer = beginSingleTimeCommands();
		vk::ImageMemoryBarrier barrier{ .oldLayout = oldLayout, .newLayout = newLayout, .image = image, .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
		barrier.subresourceRange.levelCount = mipLevels;

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
		endSingleTimeCommands(commandBuffer);
	}
	vk::CommandBuffer VulkanRenderer::beginSingleTimeCommands() {
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = vulkanCore.commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::CommandBuffer commandBuffer = vulkanCore.device.allocateCommandBuffers(allocInfo).front();
		//vk::CommandBuffer commandBuffer = std::move(vulkanCore.device.allocateCommandBuffers(allocInfo).front());

		vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		commandBuffer.begin(beginInfo);

		return commandBuffer;
	}
	void VulkanRenderer::endSingleTimeCommands(vk::CommandBuffer& commandBuffer) {
		commandBuffer.end();
		vulkanCore.queues.graphicsQueue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &commandBuffer }, nullptr);
		vulkanCore.queues.graphicsQueue.waitIdle();
	}



	void VulkanRenderer::createQueues() {
		vulkanCore.queues.graphicsQueue = vulkanCore.vkbInstances.device.get_queue(vkb::QueueType::graphics).value();
		vulkanCore.queues.presentQueue = vulkanCore.vkbInstances.device.get_queue(vkb::QueueType::present).value();
	}

	void VulkanRenderer::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = vulkanCore.vkbInstances.device.get_queue_index(vkb::QueueType::graphics).value()};

		vulkanCore.commandPool = vulkanCore.device.createCommandPool(poolInfo);
	}

	void VulkanRenderer::createDescriptorSetLayouts() {
		storageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		for (auto buffer : storageBuffers) {
			VmaAllocation bufferAllocation{};
			createBuffer(sizeof(GPUObjectData) * MAX_OBJECTS, vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferAllocation);
			storageBufferAllocations.push_back(bufferAllocation);
		}
		createGbufferDescriptorSetLayout();
	}

	void VulkanRenderer::createGbufferDescriptorSetLayout() 
	{
		std::array bindings = {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),  //Model matrix buffer
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),  // view projection buffer
			vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),  // Color texture
			vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)  // Normal texture
		};
		vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() };
		gbufferDescriptorSetLayout = vulkanCore.device.createDescriptorSetLayout(layoutInfo);
	}

	void VulkanRenderer::createDescriptorPool() {
			std::array poolSizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, static_cast<uint32_t>(storageBuffers.size())),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 100),  // Arbitrary large number for now
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100)  // Arbitrary large number for now
		};
		vk::DescriptorPoolCreateInfo poolInfo{ .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,  .maxSets = 100,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),.pPoolSizes = poolSizes.data(),  };  // Arbitrary large number for now
		descriptorPool = vulkanCore.device.createDescriptorPool(poolInfo);
	}
	
	void VulkanRenderer::createDescriptorSets() {
		createGbufferDescriptorSets();

	}

	void VulkanRenderer::createGbufferDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, gbufferDescriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pSetLayouts = layouts.data()
		};

		gbufferDescriptorSets.clear();
		gbufferDescriptorSets = vulkanCore.device.allocateDescriptorSets(allocInfo);
		
	}

	void VulkanRenderer::createDepthResources() {
		std::vector<vk::Format> candidates = {
			vk::Format::eD32Sfloat,
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint
		};

		vk::PhysicalDevice physDevice = vulkanCore.vkbInstances.device.physical_device.physical_device;
		depthImageFormat = vk::Format::eUndefined;

		for (vk::Format format : candidates) {
			vk::FormatProperties props = physDevice.getFormatProperties(format);
			if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
				depthImageFormat = format;
				break;
			}
		}

		if (depthImageFormat == vk::Format::eUndefined) {
			throw std::runtime_error("failed to find supported depth format!");
		}

		std::cout << "Chosen depth format: " << vk::to_string(depthImageFormat) << std::endl;
		createImage(vulkanCore.vkbInstances.swapChain.extent.width, vulkanCore.vkbInstances.swapChain.extent.height, 1, depthImageFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageAllocation);
		depthImageView = createImageView(depthImage, depthImageFormat, vk::ImageAspectFlagBits::eDepth, 1);
	}

	void VulkanRenderer::createGraphicsPipelines() {
		createGBufferPipeline();

	}
	void VulkanRenderer::createGBufferPipeline() {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/gbuffer.slang.spv"));
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
		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.width),
			static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.height), 0.0f, 1.0f };

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
			.logicOpEnable = VK_FALSE , .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
			.pSetLayouts = &gbufferDescriptorSetLayout,
			.pushConstantRangeCount = 0 };
		gbufferPipelineLayout = vulkanCore.device.createPipelineLayout(pipelineLayoutInfo);

		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
		};
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(vulkanCore.vkbInstances.swapChain.image_format);
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
		.layout = gbufferPipelineLayout,
		.renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat, .depthAttachmentFormat = depthImageFormat} };
		//graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	
	}

	void VulkanRenderer::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& allocation) {
		vk::ImageCreateInfo imageInfo{ .imageType = vk::ImageType::e2D, .format = format,
			.extent = {width, height, 1}, .mipLevels = mipLevels, .arrayLayers = 1,
			.samples = vk::SampleCountFlagBits::e1, .tiling = tiling,
			.usage = usage, .sharingMode = vk::SharingMode::eExclusive
		};
		imageInfo.mipLevels = mipLevels;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;


		VkImage rawImage = VK_NULL_HANDLE;
		vmaCreateImage(vulkanCore.allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageInfo), &allocInfo, &rawImage, &allocation, nullptr);

		image = vk::Image(rawImage);
	}




	void VulkanRenderer::copyBufferToImage(const vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height) {
		vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
		vk::BufferImageCopy region{ .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
		.imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, .imageOffset = {0, 0, 0}, .imageExtent = {width, height, 1} };
		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
		endSingleTimeCommands(commandBuffer);

	}
	void VulkanRenderer::generateMipmaps(vk::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
		//vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
		vk::PhysicalDevice physDevice = vulkanCore.vkbInstances.device.physical_device.physical_device;
		vk::FormatProperties formatProperties =   physDevice.getFormatProperties(imageFormat);
		if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
		vk::ImageMemoryBarrier barrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eTransferRead,
		.oldLayout = vk::ImageLayout::eTransferDstOptimal,
		.newLayout = vk::ImageLayout::eTransferSrcOptimal,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image };
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;
		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

			vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dstOffsets;
			offsets[0] = vk::Offset3D(0, 0, 0);
			offsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
			dstOffsets[0] = vk::Offset3D(0, 0, 0);
			dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
			vk::ImageBlit blit = { .srcSubresource = {}, .srcOffsets = offsets,
								.dstSubresource = {}, .dstOffsets = dstOffsets };
			blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
			blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);

			commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

		endSingleTimeCommands(commandBuffer);
	}



	/// <summary>
	/// Loads a gltf. Input a vector of mesh components to be filled with the loaded meshes, and the file path to the gltf file.
	/// </summary>
	/// <param name="meshComponents"></param>
	/// <param name="filePath"></param>
	void VulkanRenderer::loadGLTF(std::vector<MeshComponent>& meshComponents, std::string filePath) {
		//Code here based on code from: https://docs.vulkan.org/tutorial/latest/15_GLTF_KTX2_Migration.html (Accessed 17.04.2026)

		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err;
		std::string warn;
		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
		if (!warn.empty()) {
			std::cout << "GLTF Warning: " << warn << std::endl;
		}
		if (!err.empty()) {
			std::cerr << "GLTF Error: " << err << std::endl;
		}
		if (!ret) {
			throw std::runtime_error("Failed to load GLTF file: " + filePath);
		}
		//Suggstion from tutorial. Load materials and textures first so they can be referenced when loading meshes.
		//Reference from here https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Loading_Models/04_loading_gltf.html
		meshComponents.clear();
		std::vector<vk::Image> textures;
		std::vector<vk::ImageView> textureImageViews;
		std::vector<VmaAllocation> textureAllocations;
		std::vector<Material> materials;
		for (size_t i = 0; i < model.textures.size(); i++) {
			const auto& texture = model.textures[i];
			const auto& image = model.images[texture.source];
			tinygltf::Texture tex;
			tex.name = image.name.empty() ? "texture_" + std::to_string(i) : image.name;
			vk::Image textureImage{};
			if (!image.image.empty()) {
				// TinyGLTF has already decoded the PNG/JPG into raw pixel data!
				const unsigned char* dataPtr = image.image.data();
				int width = image.width;
				int height = image.height;
				const vk::DeviceSize imageSize = width * height * 4;
				vk::Buffer stagingBuffer = vk::Buffer{};
				VmaAllocation stagingAllocation = VmaAllocation{};
				createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingAllocation);
				void* mapped = nullptr;
				vmaMapMemory(vulkanCore.allocator, stagingAllocation, &mapped);
				std::memcpy(mapped, dataPtr, static_cast<size_t>(imageSize));
				vmaUnmapMemory(vulkanCore.allocator, stagingAllocation);
				vk::Image textureImage;
				VmaAllocation textureAllocation;
				uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

				createImage(width, height, mipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc |vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureAllocation);
				transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
				copyBufferToImage(stagingBuffer, textureImage, width, height);
				generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, width, height, mipLevels);
				//transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1);
				vk::ImageView textureview = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
				textures.push_back(textureImage);
				textureImageViews.push_back(textureview);
				textureAllocations.push_back(textureAllocation);
			}
			 else {
				std::cerr << "Unsupported texture format: " << image.mimeType << std::endl;
			}

		}
		for (const auto& material : model.materials) {
			Material mat;
			if (material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
				mat.baseColorFactor.r = material.pbrMetallicRoughness.baseColorFactor[0];
				mat.baseColorFactor.g = material.pbrMetallicRoughness.baseColorFactor[1];
				mat.baseColorFactor.b = material.pbrMetallicRoughness.baseColorFactor[2];
				mat.baseColorFactor.a = material.pbrMetallicRoughness.baseColorFactor[3];
			}

			mat.metallicFactor = material.pbrMetallicRoughness.metallicFactor;
			mat.roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;

			if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
				const auto& texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
				mat.baseColorTexture = textures[texture.source];
				mat.baseColorTextureView = textureImageViews[texture.source];
				mat.baseColorTextureAllocation = textureAllocations[texture.source];

			}
			if (material.normalTexture.index >= 0) {
				const auto& texture = model.textures[material.normalTexture.index];
				mat.normalTexture = textures[texture.source];
				mat.normalTextureView = textureImageViews[texture.source];
				mat.normalTextureAllocation = textureAllocations[texture.source];
			}
			materials.push_back(mat);
		}


		for (const auto& mesh : model.meshes) {
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			for (const auto& primitive : mesh.primitives) {
				MeshComponent meshComponent = {};
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

				// Get vertex positions
				const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];


				bool                        hasTexCoords = primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
				const tinygltf::Accessor* texCoordAccessor = nullptr;
				const tinygltf::BufferView* texCoordBufferView = nullptr;
				const tinygltf::Buffer* texCoordBuffer = nullptr;
				if (hasTexCoords)
				{
					texCoordAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
					texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
					texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
				}
				vertices.clear();
				indices.clear();
				uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
				for (size_t i = 0; i < posAccessor.count; i++) {
					Vertex vertex{};
					const float* posData = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset + i * posAccessor.ByteStride(posBufferView)]);
					vertex.pos = glm::vec3(posData[0], posData[1], posData[2]);
					if (hasTexCoords) {
						const float* texCoordData = reinterpret_cast<const float*>(&texCoordBuffer->data[texCoordBufferView->byteOffset + texCoordAccessor->byteOffset + i * texCoordAccessor->ByteStride(*texCoordBufferView)]);
						vertex.texCoord = glm::vec2(texCoordData[0], texCoordData[1]);
					}
					else {
						vertex.texCoord = glm::vec2(0.0f, 0.0f);
					}
					vertex.color = glm::vec3(1.0f, 1.0f, 1.0f); // Default colour
					vertices.push_back(vertex);
				}
				vk::Buffer vertexBuffer;
				createVertexBuffer(vertices, vertexBuffer);
				meshComponent.vertexBuffer = vertexBuffer;
				const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
				size_t               indexCount = indexAccessor.count;
				size_t               indexStride = 0;


				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					indexStride = sizeof(uint16_t);
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					indexStride = sizeof(uint32_t);
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					indexStride = sizeof(uint8_t);
				}
				else
				{
					throw std::runtime_error("Unsupported index component type");
				}
				indices.reserve(indices.size() + indexCount);
				for (size_t i = 0; i < indexCount; i++)
				{
					uint32_t index = 0;

					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						index = *reinterpret_cast<const uint16_t*>(indexData + i * indexStride);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						index = *reinterpret_cast<const uint32_t*>(indexData + i * indexStride);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						index = *reinterpret_cast<const uint8_t*>(indexData + i * indexStride);
					}

					indices.push_back(baseVertex + index);
				}
				vk::Buffer indBuffer;
				createIndexBuffer(indices, indBuffer);
				meshComponent.indexBuffer = indBuffer;

				if (primitive.material >= 0) {
					meshComponent.material = materials[primitive.material];
				}
				meshComponent.id = latestMeshID++;
				meshComponents.push_back(meshComponent);
			}
		}
		
	}

	void VulkanRenderer::createCommandBuffers() {
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = vulkanCore.commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT
		};
		vulkanCore.commandBuffers = vulkanCore.device.allocateCommandBuffers(allocInfo);
	}

	void VulkanRenderer::createSyncObjects() {
		for (auto &frame : vulkanCore.perFrame) {
			vk::FenceCreateInfo fenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };
			frame.renderFence = vulkanCore.device.createFence(fenceInfo);

			vk::SemaphoreCreateInfo semaphoreInfo{};
			frame.presentSemaphore = vulkanCore.device.createSemaphore(semaphoreInfo);
			frame.renderSemaphore = vulkanCore.device.createSemaphore(semaphoreInfo);
		}
	}


	void VulkanRenderer::Update(float dt) {
		drawFrame();
	}

	void VulkanRenderer::drawFrame() {
		//Get fences and semaphores for the current frame
		//Then bring in the render objects to the command buffer and submit the command buffer to the graphics queue


		//auto fenceResult = vkWaitForFences(vulkanCore.device, 1, inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
		//if (fenceResult != VK_SUCCESS)
		//{
		//	throw std::runtime_error("failed to wait for fence!");
		//}

		//auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

		//if (result == vk::Result::eErrorOutOfDateKHR)
		//{
		//	std::cout << "Swap chain is out of date, recreating swap chain..." << std::endl;
		//	recreateSwapChain();
		//	return;
		//}
		//if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		//{
		//	assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
		//	throw std::runtime_error("failed to acquire swap chain image!");
		//}

		//// Only reset the fence if we are submitting work
		//device.resetFences(*inFlightFences[frameIndex]);

		////updateUniformBuffer(frameIndex);
		//recordCommandBuffer(imageIndex);
		//vk::PipelineStageFlags waitDestinationStageMask = (vk::PipelineStageFlagBits::eColorAttachmentOutput);
		//const vk::SubmitInfo   submitInfo{ .waitSemaphoreCount = 1,
		//						  .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
		//						  .pWaitDstStageMask = &waitDestinationStageMask,
		//						  .commandBufferCount = 1,
		//						  .pCommandBuffers = &*commandBuffers[frameIndex],
		//						  .signalSemaphoreCount = 1,
		//						  .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] };

		//graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

		//const vk::PresentInfoKHR presentInfo{
		//	.waitSemaphoreCount = 1,
		//	.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
		//	.swapchainCount = 1,
		//	.pSwapchains = &*swapChain,
		//	.pImageIndices = &imageIndex,
		//	.pResults = nullptr
		//};
		////result = graphicsQueue.presentKHR(presentInfo);
		////This is dumb. The c++ bindings throw an exception if the result isn't esuccess or suboptimal.
		//result = static_cast<vk::Result>(
		//	graphicsQueue.getDispatcher()->vkQueuePresentKHR(static_cast<VkQueue>(*graphicsQueue), reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo)));


		//if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR) || framebufferResized) {
		//	std::cout << "Swap chain is out of date or suboptimal, recreating swap chain..." << std::endl;
		//	framebufferResized = false;
		//	recreateSwapChain();
		//}
		//else {
		//	assert(result == vk::Result::eSuccess);
		// }
		//frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;



	}

}