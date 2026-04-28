#include "VulkanRenderer.h"
//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "tiny_gltf_v3.h"
#include "vma/vk_mem_alloc.h"

#include "stb_image.h"

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
		if (vulkanCore.device) {
			try {
				vulkanCore.device.waitIdle();
			}
			catch (const vk::SystemError& e) {
				std::cerr << "[Vulkan Cleanup] waitIdle failed: " << e.what() << "\n";
			}
		}
		quad.Destroy(vulkanCore.device, vulkanCore.allocator);

		if (skybox.skyboxImage){
			vmaDestroyImage(vulkanCore.allocator, static_cast<VkImage>(skybox.skyboxImage), skybox.skyboxAllocation);
			vmaDestroyImage(vulkanCore.allocator, static_cast<VkImage>(skybox.skyboxRenderOutputImage), skybox.skyboxRenderOutputAllocation);
			vulkanCore.device.destroyImageView(skybox.skyboxImageView);
			vulkanCore.device.destroyImageView(skybox.skyboxRenderOutputView);
			skybox.skyboxImage = nullptr;
			skybox.skyboxRenderOutputImage = nullptr;
			skybox.skyboxAllocation = nullptr;
			skybox.skyboxRenderOutputAllocation = nullptr;
			skybox.skyboxImageView = nullptr;
			skybox.skyboxRenderOutputView = nullptr;
		}
		if (gBuffer.gbufferImage) {
			vmaDestroyImage(vulkanCore.allocator, static_cast<VkImage>(gBuffer.gbufferImage), gBuffer.gbufferAllocation);
			vulkanCore.device.destroyImageView(gBuffer.gbufferImageView);
			gBuffer.gbufferImage = nullptr;
			gBuffer.gbufferAllocation = nullptr;
			gBuffer.gbufferImageView = nullptr;
		}

		// 1) Sync objects
		for (auto& frame : vulkanCore.perFrame) {
			if (frame.renderFence) { vulkanCore.device.destroyFence(frame.renderFence);          frame.renderFence = nullptr; }
			if (frame.presentSemaphore) { vulkanCore.device.destroySemaphore(frame.presentSemaphore); frame.presentSemaphore = nullptr; }
		}
		for (auto& sem : vulkanCore.renderSemaphores) {
			if (sem) vulkanCore.device.destroySemaphore(sem);
		}
		vulkanCore.renderSemaphores.clear();

		// 2) Pipeline & layout
		if (gBuffer.gBufferPipeline) { vulkanCore.device.destroyPipeline(gBuffer.gBufferPipeline);             gBuffer.gBufferPipeline = nullptr; }
		if (gBuffer.gbufferPipelineLayout) { vulkanCore.device.destroyPipelineLayout(gBuffer.gbufferPipelineLayout); gBuffer.gbufferPipelineLayout = nullptr; }

		if (skybox.skyboxPipeline) { vulkanCore.device.destroyPipeline(skybox.skyboxPipeline); skybox.skyboxPipeline = nullptr; }
		if (skybox.skyboxPipelineLayout) { vulkanCore.device.destroyPipelineLayout(skybox.skyboxPipelineLayout); skybox.skyboxPipelineLayout = nullptr; }
		
		if (finalOutput.finalOutputPipeline) { vulkanCore.device.destroyPipeline(finalOutput.finalOutputPipeline); finalOutput.finalOutputPipeline = nullptr; }
		if (finalOutput.finalOutputPipelineLayout) { vulkanCore.device.destroyPipelineLayout(finalOutput.finalOutputPipelineLayout); finalOutput.finalOutputPipelineLayout = nullptr; }

		// 3) Descriptor pool (implicitly frees all descriptor sets), then layout
		if (descriptorPool) { vulkanCore.device.destroyDescriptorPool(descriptorPool);                descriptorPool = nullptr; }
		if (objectDescriptorSetLayout) { vulkanCore.device.destroyDescriptorSetLayout(objectDescriptorSetLayout); objectDescriptorSetLayout = nullptr; }
		if (skybox.skyboxDescriptorSetLayout) { vulkanCore.device.destroyDescriptorSetLayout(skybox.skyboxDescriptorSetLayout); skybox.skyboxDescriptorSetLayout = nullptr; }
		if (finalOutput.finalOutputDescriptorSetLayout) { vulkanCore.device.destroyDescriptorSetLayout(finalOutput.finalOutputDescriptorSetLayout); finalOutput.finalOutputDescriptorSetLayout = nullptr; }
		// 4) Sampler
		if (vulkanCore.textureSampler) { vulkanCore.device.destroySampler(vulkanCore.textureSampler); vulkanCore.textureSampler = nullptr; }

		// 5) VMA buffers (camera & storage)
		for (size_t i = 0; i < cameraBuffers.size(); ++i) {
			if (cameraBuffers[i]) vmaDestroyBuffer(vulkanCore.allocator, static_cast<VkBuffer>(cameraBuffers[i]), cameraBufferAllocations[i]);
		}
		cameraBuffers.clear();
		cameraBufferAllocations.clear();

		for (size_t i = 0; i < storageBuffers.size(); ++i) {
			if (storageBuffers[i]) vmaDestroyBuffer(vulkanCore.allocator, static_cast<VkBuffer>(storageBuffers[i]), storageBufferAllocations[i]);
		}
		storageBuffers.clear();
		storageBufferAllocations.clear();

		// 6) Depth image view, then depth image
		if (depthImageView) { vulkanCore.device.destroyImageView(depthImageView); depthImageView = nullptr; }
		if (depthImage) { vmaDestroyImage(vulkanCore.allocator, static_cast<VkImage>(depthImage), depthImageAllocation); depthImage = nullptr; depthImageAllocation = nullptr; }

		// 7) Swapchain image views
		for (auto& iv : vulkanCore.swapChainImageViews) {
			if (iv) vulkanCore.device.destroyImageView(iv);
		}
		vulkanCore.swapChainImageViews.clear();
		vulkanCore.swapChainImages.clear();

		// 8) Command pool (implicitly frees all command buffers)
		std::cerr << ">> commandPool handle: " << (VkCommandPool)vulkanCore.commandPool << "\n";
		if (vulkanCore.commandPool) {
			vulkanCore.device.destroyCommandPool(vulkanCore.commandPool);
			vulkanCore.commandPool = nullptr;
			std::cerr << ">> commandPool destroyed\n";
		}
		std::cerr << ">> past commandPool\n";
		// 9) VMA allocator
		if (vulkanCore.allocator) { vmaDestroyAllocator(vulkanCore.allocator); vulkanCore.allocator = nullptr; }

		// 10) Swapchain  device  surface  instance (vk-bootstrap)
		if (vulkanCore.vkbInstances.swapChain) vkb::destroy_swapchain(vulkanCore.vkbInstances.swapChain);
		if (vulkanCore.vkbInstances.device)    vkb::destroy_device(vulkanCore.vkbInstances.device);
		if (vulkanCore.surface)                vkb::destroy_surface(vulkanCore.instance, vulkanCore.surface);
		if (vulkanCore.instance)               vkb::destroy_instance(vulkanCore.instance);

		// Null all vkb wrappers so their destructors are no-ops when VulkanCore goes out of scope
		vulkanCore.vkbInstances.swapChain = vkb::Swapchain{};
		vulkanCore.vkbInstances.device = vkb::Device{};
		vulkanCore.vkbInstances.surface = nullptr;
		vulkanCore.instance = vkb::Instance{};
		vulkanCore.surface = nullptr;
		vulkanCore.swapChain = nullptr;
		vulkanCore.device = nullptr;
	}


	void VulkanRenderer::DestroyAllRenderables()
	{
		std::vector<RenderableComponent>* renderObjects = gameworld.getallRenderableComponents();
		for (size_t i = 0; i < renderObjects->size(); ++i) {
			if (renderObjects->at(i).mesh) {
				for (auto& meshComp : *renderObjects->at(i).mesh) {
					meshComp.Destroy(vulkanCore.device, vulkanCore.allocator);
				}
				// Keep the container lifetime as-is (we only free GPU resources here)
			}
		}

		// Free the temporary container allocated by Gameworld
		delete renderObjects;
		renderObjects = nullptr;
	} 

	void VulkanRenderer::DestroyMesh(std::vector<MeshComponent>& mesh) {
		for (auto& meshComp : mesh) {
			meshComp.Destroy(vulkanCore.device, vulkanCore.allocator);
		}
		mesh.clear();
	}

	void VulkanRenderer::cleanupSwapChain() {
		vulkanCore.device.destroyImageView(depthImageView);
		vmaDestroyImage(vulkanCore.allocator, depthImage, depthImageAllocation);

		for (auto imageView : vulkanCore.swapChainImageViews) {
			vulkanCore.device.destroyImageView(imageView);
		}
		vulkanCore.swapChainImageViews.clear();
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
			auto inst_ret = builder.set_app_name("JDEngine").request_validation_layers().require_api_version(1, 3)
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
			//createCommandPool();
			createQueues();
			createImageViews();
			createDescriptorSetLayouts();
			createDescriptorPool();
			createDepthResources();
			createTextureSampler();
			createGraphicsPipelines();
			createCameraBuffers();
			createCommandPool();
			createQuad();
			loadSkybox();

			//createDescriptorSets();
			createCommandBuffers();
			createSyncObjects();
			createGBufferImages();
			createOutputDescriptorSets();
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
			.set_required_features(JD::RequiredPhysicalFeatures().features.features)
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

		auto swap_ret = swapchainBuilder.set_old_swapchain(vulkanCore.swapChain).set_desired_format(desiredFormat)
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

	vk::ImageView VulkanRenderer::createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, vk::ImageViewType viewType, uint32_t layerCount) {
		vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = viewType,
			.format = format, .subresourceRange = { aspectFlags, 0, 1, 0, layerCount } };
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

	void VulkanRenderer::createTextureSampler() {
		vk::PhysicalDevice physDevice = vulkanCore.vkbInstances.device.physical_device.physical_device;
		vk::PhysicalDeviceProperties properties = physDevice.getProperties();
		vk::SamplerCreateInfo samplerInfo{ .magFilter = vk::Filter::eLinear, .minFilter = vk::Filter::eLinear,  .mipmapMode = vk::SamplerMipmapMode::eLinear,
			.addressModeU = vk::SamplerAddressMode::eRepeat, .addressModeV = vk::SamplerAddressMode::eRepeat, .addressModeW = vk::SamplerAddressMode::eRepeat,
			.anisotropyEnable = vk::True, .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = vk::False, .compareOp = vk::CompareOp::eAlways };
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerInfo.unnormalizedCoordinates = vk::False;
		samplerInfo.compareEnable = vk::False;
		samplerInfo.compareOp = vk::CompareOp::eAlways;

		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = vk::LodClampNone;
		vulkanCore.textureSampler = vulkanCore.device.createSampler(samplerInfo);
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

	void VulkanRenderer::createVertexBuffer(std::vector<Vertex>& verticies, vk::Buffer& buffer, VmaAllocation& allocation) {
		vk::DeviceSize bufferSize = sizeof(verticies[0]) * verticies.size();
		vk::Buffer stagingBuffer = vk::Buffer{};
		VmaAllocation stagingBufferMemory = VmaAllocation{};
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
		void* mapped = nullptr;
		vmaMapMemory(vulkanCore.allocator, stagingBufferMemory, &mapped);
		std::memcpy(mapped, verticies.data(), (size_t)bufferSize);
		vmaUnmapMemory(vulkanCore.allocator, stagingBufferMemory);
		//VmaAllocation vertexBufferMemory = VmaAllocation{};
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, allocation);
		copyBuffer(stagingBuffer, buffer, bufferSize);
		vmaDestroyBuffer(vulkanCore.allocator, static_cast<VkBuffer>(stagingBuffer), stagingBufferMemory);


	}

	void VulkanRenderer::createIndexBuffer(std::vector<uint32_t>& indicies, vk::Buffer& buffer, VmaAllocation& allocation) {
		vk::DeviceSize bufferSize = sizeof(indicies[0]) * indicies.size();
		vk::Buffer stagingBuffer = vk::Buffer{};
		VmaAllocation stagingBufferMemory = VmaAllocation{};
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
		void* mapped = nullptr;
		vmaMapMemory(vulkanCore.allocator, stagingBufferMemory, &mapped);
		std::memcpy(mapped, indicies.data(), (size_t)bufferSize);
		vmaUnmapMemory(vulkanCore.allocator, stagingBufferMemory);
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, allocation);
		copyBuffer(stagingBuffer, buffer, bufferSize);
		vmaDestroyBuffer(vulkanCore.allocator, static_cast<VkBuffer>(stagingBuffer), stagingBufferMemory);

	}

	void VulkanRenderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, VmaAllocation& allocation) {
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

	void VulkanRenderer::createQuad() {
		quad = {};
		/*std::vector<Vertex> vertices = {
			{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
			{{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
			{{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
			{{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}}
		};*/
		std::vector<Vertex> vertices = {
		Vertex{{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		Vertex{{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		Vertex{{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		Vertex{{-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
		};

		std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
		createVertexBuffer(vertices, quad.vertexBuffer, quad.vertexBufferAllocation);
		createIndexBuffer(indices, quad.indexBuffer, quad.indexBufferAllocation);
		quad.vertices = std::move(vertices);
		quad.indices = std::move(indices);
	}


	void VulkanRenderer::createCameraBuffers() {
		cameraBuffers.clear();
		cameraBufferAllocations.clear();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::Buffer buffer;
			VmaAllocation allocation;
			createBuffer(sizeof(CameraInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, allocation);
			cameraBuffers.push_back(buffer);
			cameraBufferAllocations.push_back(allocation);
		}
	}

	void VulkanRenderer::transitionImageLayout(vk::CommandBuffer& commandBuffer, vk::Image image,
		vk::ImageLayout       old_layout,
		vk::ImageLayout         new_layout,
		vk::AccessFlags2        src_access_mask,
		vk::AccessFlags2        dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask,
		vk::ImageAspectFlags image_aspect_flags,
		uint32_t mip_levels,
		uint32_t layer_count) {
		
		vk::ImageMemoryBarrier2 barrier{
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = image_aspect_flags,
				.baseMipLevel = 0,
				.levelCount = mip_levels,
				.baseArrayLayer = 0,
				.layerCount = layer_count} };

		vk::DependencyInfo dependencyInfo{
			.dependencyFlags = {},
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier
		};
		commandBuffer.pipelineBarrier2(dependencyInfo);
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
		for (auto& buffer : storageBuffers) {
			VmaAllocation bufferAllocation{};
			createBuffer(sizeof(GPUObjectData) * MAX_OBJECTS, vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferAllocation);
			storageBufferAllocations.push_back(bufferAllocation);
		}
		createSkyboxDescriptorSetLayout();
		createObjectDescriptorSetLayouts();
		createOutputDescriptorSetLayout();
	}

	void VulkanRenderer::createSkyboxDescriptorSetLayout() {
		std::array bindings = {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),  // view projection buffer
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)  // Cubemap texture
		};
		vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() };
		skybox.skyboxDescriptorSetLayout = vulkanCore.device.createDescriptorSetLayout(layoutInfo);
	}


	void VulkanRenderer::createObjectDescriptorSetLayouts() 
	{
		std::array bindings = {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),  //Model matrix buffer
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),  // view projection buffer
			vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),  // Color texture
			vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)  // Normal texture
		};
		vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() };
		objectDescriptorSetLayout = vulkanCore.device.createDescriptorSetLayout(layoutInfo);
	}

	void VulkanRenderer::createOutputDescriptorSetLayout() 
	{
		std::array bindings = {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),  // G-buffer color texture
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)  // Skybox
		};
		vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() };
		finalOutput.finalOutputDescriptorSetLayout = vulkanCore.device.createDescriptorSetLayout(layoutInfo);
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
	
	/*void VulkanRenderer::createDescriptorSets() {
		createGbufferDescriptorSets();

	}*/

	/*void VulkanRenderer::createGbufferDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, gbufferDescriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pSetLayouts = layouts.data()
		};

		gbufferDescriptorSets.clear();
		gbufferDescriptorSets = vulkanCore.device.allocateDescriptorSets(allocInfo);
		
	}*/

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
		createSkyboxPipeline();
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

		// --- Push constant configuration added ---
		vk::PushConstantRange pushConstantRange{
			.stageFlags = vk::ShaderStageFlagBits::eVertex,
			.offset = 0,
			.size = sizeof(uint32_t)
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1,
			.pSetLayouts = &objectDescriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange };
		gBuffer.gbufferPipelineLayout = vulkanCore.device.createPipelineLayout(pipelineLayoutInfo);

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
		.layout = gBuffer.gbufferPipelineLayout,
		.renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat, .depthAttachmentFormat = depthImageFormat} };
		
		// --- Fix missing pipeline assignment ---
		auto pipelineResult = vulkanCore.device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create gbuffer pipeline!");
		}
		gBuffer.gBufferPipeline = pipelineResult.value;
		vulkanCore.device.destroyShaderModule(shaderModule);
		
	}

	void VulkanRenderer::createSkyboxPipeline() {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/skybox.slang.spv"));
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

		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.width),
		static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.height), 0.0f, 1.0f };

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
		.pSetLayouts = &skybox.skyboxDescriptorSetLayout };
		skybox.skyboxPipelineLayout = vulkanCore.device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(vulkanCore.vkbInstances.swapChain.image_format);

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
		 .layout = skybox.skyboxPipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat} };

		auto pipelineResult = vulkanCore.device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create skybox pipeline!");
		}
		skybox.skyboxPipeline = pipelineResult.value;
		vulkanCore.device.destroyShaderModule(shaderModule);

	}


	void VulkanRenderer::createCubeMapTextureImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& allocation) {
		vk::ImageCreateInfo imageInfo{ .imageType = vk::ImageType::e2D, .format = format,
			.extent = {width, height, 1}, .mipLevels = mipLevels, .arrayLayers = 6,
			.samples = vk::SampleCountFlagBits::e1, .tiling = tiling,
			.usage = usage, .sharingMode = vk::SharingMode::eExclusive,
		};
		imageInfo.mipLevels = mipLevels;
		imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;


		VkImage rawImage = VK_NULL_HANDLE;
		vmaCreateImage(vulkanCore.allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageInfo), &allocInfo, &rawImage, &allocation, nullptr);

		image = vk::Image(rawImage);

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

	void VulkanRenderer::copyBufferToImage(const vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height, uint32_t layerCount) {
		vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
		vk::BufferImageCopy region{ .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
		.imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, layerCount }, .imageOffset = {0, 0, 0}, .imageExtent = {width, height, 1} };
		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
		endSingleTimeCommands(commandBuffer);

	}
	void VulkanRenderer::generateMipmaps(vk::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount) {
		vk::PhysicalDevice physDevice = vulkanCore.vkbInstances.device.physical_device.physical_device;
		vk::FormatProperties formatProperties = physDevice.getFormatProperties(imageFormat);
		if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

		// For each layer, generate mipmaps by blitting downwards
		for (uint32_t layer = 0; layer < layerCount; ++layer) {
			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			// Transition level 0 of this layer from TRANSFER_DST_OPTIMAL -> TRANSFER_SRC_OPTIMAL when needed inside loop
			for (uint32_t i = 1; i < mipLevels; i++) {
				// Barrier for src (previous mip) before blit
				vk::ImageMemoryBarrier barrierSrc{
					.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
					.dstAccessMask = vk::AccessFlagBits::eTransferRead,
					.oldLayout = vk::ImageLayout::eTransferDstOptimal,
					.newLayout = vk::ImageLayout::eTransferSrcOptimal,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = image
				};
				barrierSrc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				barrierSrc.subresourceRange.baseMipLevel = i - 1;
				barrierSrc.subresourceRange.levelCount = 1;
				barrierSrc.subresourceRange.baseArrayLayer = layer;
				barrierSrc.subresourceRange.layerCount = 1; // Must process 1 layer at a time

				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrierSrc);

				vk::ArrayWrapper1D<vk::Offset3D, 2> srcOffsets, dstOffsets;
				srcOffsets[0] = vk::Offset3D(0, 0, 0);
				srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
				dstOffsets[0] = vk::Offset3D(0, 0, 0);
				dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

				vk::ImageBlit blit{};
				blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, layer, 1);
				blit.srcOffsets = srcOffsets;
				blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, layer, 1);
				blit.dstOffsets = dstOffsets;

				commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

				// Barrier for src mip after blit to transition to SHADER_READ_ONLY_OPTIMAL for fragment shader reading
				vk::ImageMemoryBarrier barrierDst{
					.srcAccessMask = vk::AccessFlagBits::eTransferRead,
					.dstAccessMask = vk::AccessFlagBits::eShaderRead,
					.oldLayout = vk::ImageLayout::eTransferSrcOptimal, // Updated from TransferDstOptimal
					.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = image
				};
				barrierDst.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				barrierDst.subresourceRange.baseMipLevel = i - 1;
				barrierDst.subresourceRange.levelCount = 1;
				barrierDst.subresourceRange.baseArrayLayer = layer;
				barrierDst.subresourceRange.layerCount = 1; // Must process 1 layer at a time

				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrierDst);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			// Transition the last mip level to SHADER_READ_ONLY_OPTIMAL
			vk::ImageMemoryBarrier barrierLast{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.oldLayout = vk::ImageLayout::eTransferDstOptimal,
				.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image
			};
			barrierLast.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			barrierLast.subresourceRange.baseMipLevel = mipLevels - 1;
			barrierLast.subresourceRange.levelCount = 1;
			barrierLast.subresourceRange.baseArrayLayer = layer;
			barrierLast.subresourceRange.layerCount = 1; // Must process 1 layer at a time

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrierLast);
		}

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
		/*std::unordered_map<int, vk::Image> textures;
		std::unordered_map<int, vk::ImageView> textureImageViews;
		std::unordered_map<int, VmaAllocation> textureAllocations;*/
		std::vector<tinygltf::Image> images;
		std::vector<Material> materials;
		for (size_t i = 0; i < model.textures.size(); i++) {
			const auto& texture = model.textures[i];
			const auto& image = model.images[texture.source];
			tinygltf::Texture tex;
			tex.name = image.name.empty() ? "texture_" + std::to_string(i) : image.name;
			//To detect if its the normal map. Compare the normal texture index to the texture index.
			if (!image.image.empty()) {
				images.push_back(image);
			}
				// TinyGLTF has already decoded the PNG/JPG into raw pixel data!
		}
		for (const auto& material : model.materials) {
			Material mat;

		/*	const auto& texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
			std::cout << "baseColor texture.source = " << texture.source << std::endl;
			std::cout << "baseColorTextureView handle = "
				<< (VkImageView)textureImageViews[texture.source] << std::endl;
			std::cout << "normalTextureView handle = "
				<< (VkImageView)textureImageViews[model.textures[material.normalTexture.index].source] << std::endl;*/


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
				tinygltf::Image image = model.images[texture.source];
				vk::Format format = vk::Format::eR8G8B8A8Srgb;

				if (image.bits == 16) {
					format = vk::Format::eR16G16B16A16Unorm;
					std::cout << "Image " << texture.source << " has 16 bits per channel, using R16G16B16A16Sfloat format." << std::endl;

					// Decode sRGB gamma to linear before uploading
					auto* pixels = reinterpret_cast<uint16_t*>(image.image.data());
					int pixelCount = image.width * image.height;
					for (int p = 0; p < pixelCount; ++p) {
						for (int c = 0; c < 3; ++c) {  // R, G, B only — not alpha
							float srgb = pixels[p * 4 + c] / 65535.0f;
							float linear = srgb <= 0.04045f
								? srgb / 12.92f
								: powf((srgb + 0.055f) / 1.055f, 2.4f);
							pixels[p * 4 + c] = static_cast<uint16_t>(linear * 65535.0f);
						}
					}
				}

				createVkImageFromGLTFImage(mat.baseColorTexture, mat.baseColorTextureView, mat.baseColorTextureAllocation, image, format);
			}

			if (material.normalTexture.index >= 0) {
				const auto& texture = model.textures[material.normalTexture.index];
				tinygltf::Image image = model.images[texture.source];
				vk::Format format = vk::Format::eR8G8B8A8Unorm; // Assuming the image is in RGBA8 format. In a real implementation, you'd want to check the actual format of the image data.
				if (image.bits == 16) {
					format = vk::Format::eR16G16B16A16Unorm; // If the image has 16 bits per channel, use a 16-bit format
					std::cout << "Image " << texture.source << " has 16 bits per channel, using R16G16B16A16Unorm format." << std::endl;
				}
				createVkImageFromGLTFImage(mat.normalTexture, mat.normalTextureView, mat.normalTextureAllocation, image, format);
			}
			std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, objectDescriptorSetLayout);
			vk::DescriptorSetAllocateInfo allocInfo{
				.descriptorPool = descriptorPool,
				.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
				.pSetLayouts = layouts.data()
			};
			mat.descriptorSets.clear();
			mat.descriptorSets = vulkanCore.device.allocateDescriptorSets(allocInfo);
			for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
				vk::DescriptorBufferInfo bufferInfo{ .buffer = storageBuffers[i], .offset = 0, .range = sizeof(GPUObjectData) * MAX_OBJECTS };
				vk::DescriptorBufferInfo cameraBufferInfo{ .buffer = cameraBuffers[i], .offset = 0, .range = sizeof(CameraInfo) };
				vk::DescriptorImageInfo baseColorImageInfo{ .sampler = vulkanCore.textureSampler, .imageView = mat.baseColorTextureView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
				vk::DescriptorImageInfo normalImageInfo{ .sampler = vulkanCore.textureSampler, .imageView = mat.normalTextureView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
				//std::cout << "baseColorTextureView: " << (VkImageView)mat.baseColorTextureView << std::endl;
				//std::cout << "normalTextureView:    " << (VkImageView)mat.normalTextureView << std::endl;
				std::array<vk::WriteDescriptorSet, 4> descriptorWrites = {
						vk::WriteDescriptorSet {
							.dstSet = mat.descriptorSets[i],
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = vk::DescriptorType::eStorageBuffer,
							.pBufferInfo = &bufferInfo
						},
						vk::WriteDescriptorSet {
							.dstSet = mat.descriptorSets[i],
							.dstBinding = 1,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = vk::DescriptorType::eUniformBuffer,
							.pBufferInfo = &cameraBufferInfo
						},
						vk::WriteDescriptorSet {
							.dstSet = mat.descriptorSets[i],
							.dstBinding = 2,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = vk::DescriptorType::eCombinedImageSampler,
							.pImageInfo = &baseColorImageInfo
						},
						vk::WriteDescriptorSet {
							.dstSet = mat.descriptorSets[i],
							.dstBinding = 3,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = vk::DescriptorType::eCombinedImageSampler,
							.pImageInfo = &normalImageInfo
						}
									};
				vulkanCore.device.updateDescriptorSets(descriptorWrites, {});
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
				VmaAllocation vertexBufferAllocation;
				createVertexBuffer(vertices, vertexBuffer, vertexBufferAllocation);
				meshComponent.vertexBuffer = vertexBuffer;
				meshComponent.vertexBufferAllocation = vertexBufferAllocation;
				meshComponent.vertices = std::move(vertices);
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
				VmaAllocation indBufferAllocation;
				createIndexBuffer(indices, indBuffer, indBufferAllocation);
				meshComponent.indexBuffer = indBuffer;
				meshComponent.indexBufferAllocation = indBufferAllocation;
				meshComponent.indices = std::move(indices);
				if (primitive.material >= 0) {
					meshComponent.material = materials[primitive.material];
				}
				meshComponent.id = latestMeshID++;
				meshComponents.push_back(meshComponent);
			}
		}
		
	}

	void VulkanRenderer::createVkImageFromGLTFImage(vk::Image& image, vk::ImageView& imageView,VmaAllocation& allocation, tinygltf::Image& gltfImage, vk::Format format) {
		const unsigned char* dataPtr = gltfImage.image.data();
		int width = gltfImage.width;
		int height = gltfImage.height;
		int components = gltfImage.component;
		const vk::DeviceSize imageSize =gltfImage.bits==16? width * height * 4 * 2 : width * height * 4;
		//if (gltfImage.bits == 16) {
		//	imageSize *= 2; // Each channel is 2 bytes instead of 1
		//}
		std::vector<unsigned char> expandedData;
		if (components == 3) {
			expandedData.resize(width * height * 4);
			for (int i = 0; i < width * height; ++i) {
				expandedData[i * 4 + 0] = dataPtr[i * 3 + 0]; // R
				expandedData[i * 4 + 1] = dataPtr[i * 3 + 1]; // G
				expandedData[i * 4 + 2] = dataPtr[i * 3 + 2]; // B
				expandedData[i * 4 + 3] = 255;                 // A (fully opaque)
			}
			dataPtr = expandedData.data();
		}
		else if (components != 4) {
			std::cerr << "Unexpected component count: " << components << std::endl;
			// or handle grayscale etc.
		}
		//Check bits when selecting before using this function
		//if (image.bits == 16) {
		//	// Convert 16-bit channels down to 8-bit
		//	expandedData.resize(width * height * 4);
		//	const uint16_t* src = reinterpret_cast<const uint16_t*>(dataPtr);
		//	for (int i = 0; i < width * height * 4; ++i) {
		//		expandedData[i] = static_cast<unsigned char>(src[i] >> 8); // take high byte
		//	}
		//	dataPtr = expandedData.data();

		//}
		vk::Buffer stagingBuffer = vk::Buffer{};
		VmaAllocation stagingAllocation = VmaAllocation{};
		createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingAllocation);
		void* mapped = nullptr;
		int centerPixel = (height / 2 * width + width / 2);
		int offset = centerPixel * (components == 3 ? 3 : 4);
		vmaMapMemory(vulkanCore.allocator, stagingAllocation, &mapped);
		std::memcpy(mapped, dataPtr, static_cast<size_t>(imageSize));
		vmaUnmapMemory(vulkanCore.allocator, stagingAllocation);
		vk::Image textureImage{};
		VmaAllocation textureAllocation{};
		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		createImage(width, height, mipLevels, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureAllocation);
		vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
		transitionImageLayout(commandBuffer,
			textureImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			{},
			vk::AccessFlagBits2::eTransferWrite,
			vk::PipelineStageFlagBits2::eTopOfPipe,
			vk::PipelineStageFlagBits2::eTransfer,
			vk::ImageAspectFlagBits::eColor,
			mipLevels
		);
		endSingleTimeCommands(commandBuffer);
		copyBufferToImage(stagingBuffer, textureImage, width, height);
		generateMipmaps(textureImage, format, width, height, mipLevels);
		vmaDestroyBuffer(vulkanCore.allocator, static_cast<VkBuffer>(stagingBuffer), stagingAllocation);

		//transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1);
		vk::ImageView textureview = createImageView(textureImage, format, vk::ImageAspectFlagBits::eColor, mipLevels);
		image = textureImage;
		imageView = textureview;
		allocation = textureAllocation;
		/*else {
			std::cerr << "Unsupported texture format: " << image.mimeType << std::endl;
		}*/
	
	}


	void VulkanRenderer::loadCubemap(std::vector<std::string> faces, vk::Image& cubemapImage, VmaAllocation& cubemapAllocation, vk::ImageView& cubemapImageView) 
	{
		int width, height, channels;
		std::array<unsigned char*, 6> data;
		for (size_t i = 0; i < faces.size(); i++) {
			data[i] = stbi_load(faces[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (!data[i]) {
				throw std::runtime_error("Failed to load cubemap texture: " + faces[i]);
			}
		}
		const vk::DeviceSize imageSize = width * height * 4*6;
		vk::Buffer stagingBuffer;
		VmaAllocation stagingAllocation;
		createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingAllocation);
		void* mapped;
		vmaMapMemory(vulkanCore.allocator, stagingAllocation, &mapped);
		for (size_t i = 0; i < faces.size(); i++) {
			std::memcpy(static_cast<unsigned char*>(mapped) + i * width * height * 4, data[i], static_cast<size_t>(width * height * 4));
		}
		vmaUnmapMemory(vulkanCore.allocator, stagingAllocation);
		for (size_t i = 0; i < faces.size(); i++) {
			stbi_image_free(data[i]);
		}
		float mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		createCubeMapTextureImage(width, height, mipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eTransferSrc |vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, cubemapImage, cubemapAllocation);
		vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
		transitionImageLayout(commandBuffer,
			cubemapImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			{},
			vk::AccessFlagBits2::eTransferWrite,
			vk::PipelineStageFlagBits2::eTopOfPipe,
			vk::PipelineStageFlagBits2::eTransfer,
			vk::ImageAspectFlagBits::eColor,
			mipLevels,
			6
		);
		endSingleTimeCommands(commandBuffer);
		copyBufferToImage(stagingBuffer, cubemapImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 6);
		generateMipmaps(cubemapImage, vk::Format::eR8G8B8A8Srgb, width, height, mipLevels ,6);
		vmaDestroyBuffer(vulkanCore.allocator, stagingBuffer, stagingAllocation);
		cubemapImageView =std::move(createImageView(cubemapImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels, vk::ImageViewType::eCube, 6));
	}



	void VulkanRenderer::loadSkybox() {

		loadCubemap({
			IMAGEDIR"/skybox/CubeMapLeft.jpg",
			IMAGEDIR"/skybox/CubeMapRight.jpg",
			IMAGEDIR"/skybox/CubeMapUp.jpg",
			IMAGEDIR"/skybox/CubeMapDown.jpg",
			IMAGEDIR"/skybox/CubeMapBack.jpg",
			IMAGEDIR"/skybox/CubeMapFront.jpg"
			},skybox.skyboxImage ,skybox.skyboxAllocation, skybox.skyboxImageView);
		
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, skybox.skyboxDescriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pSetLayouts = layouts.data()
		};
		skybox.skyboxDescriptorSets.clear();
		skybox.skyboxDescriptorSets = vulkanCore.device.allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::DescriptorBufferInfo cameraBufferInfo{ .buffer = cameraBuffers[i], .offset = 0, .range = sizeof(CameraInfo) };
			vk::DescriptorImageInfo imageInfo{ .sampler = vulkanCore.textureSampler, .imageView = skybox.skyboxImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
			std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
				vk::WriteDescriptorSet {
					.dstSet = skybox.skyboxDescriptorSets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pBufferInfo = &cameraBufferInfo
				},
				vk::WriteDescriptorSet {
					.dstSet = skybox.skyboxDescriptorSets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &imageInfo
				}
			};
			vulkanCore.device.updateDescriptorSets(descriptorWrites, {});


		}
		vk::Format renderOutputFormat = static_cast<vk::Format>(vulkanCore.vkbInstances.swapChain.image_format);

		createImage(vulkanCore.vkbInstances.swapChain.extent.width, vulkanCore.vkbInstances.swapChain.extent.height, 1, renderOutputFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, skybox.skyboxRenderOutputImage, skybox.skyboxRenderOutputAllocation);
		skybox.skyboxRenderOutputView = createImageView(skybox.skyboxRenderOutputImage, renderOutputFormat, vk::ImageAspectFlagBits::eColor, 1);

	}

	void VulkanRenderer::createGBufferImages() {
		vk::Format gbufferFormat = vk::Format::eB8G8R8A8Srgb;
		createImage(vulkanCore.vkbInstances.swapChain.extent.width, vulkanCore.vkbInstances.swapChain.extent.height, 1, gbufferFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, gBuffer.gbufferImage, gBuffer.gbufferAllocation);
		gBuffer.gbufferImageView = createImageView(gBuffer.gbufferImage, gbufferFormat, vk::ImageAspectFlagBits::eColor, 1);
	}


	void VulkanRenderer::createOutputDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, finalOutput.finalOutputDescriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pSetLayouts = layouts.data()
		};
		finalOutput.finalOutputDescriptorSets.clear();
		finalOutput.finalOutputDescriptorSets = vulkanCore.device.allocateDescriptorSets(allocInfo);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
		{
			vk::DescriptorImageInfo deferredImageInfo{ .sampler = vulkanCore.textureSampler, .imageView = gBuffer.gbufferImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo skyboxImageInfo{ .sampler = vulkanCore.textureSampler, .imageView = skybox.skyboxRenderOutputView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
			std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
				vk::WriteDescriptorSet{
				.dstSet = finalOutput.finalOutputDescriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eCombinedImageSampler,
				.pImageInfo = &deferredImageInfo
				},
				vk::WriteDescriptorSet{
				.dstSet = finalOutput.finalOutputDescriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eCombinedImageSampler,
				.pImageInfo = &skyboxImageInfo
				}
			};
			vulkanCore.device.updateDescriptorSets(descriptorWrites, {});
		}
		createOutputPipelineLayout();
	}

	void VulkanRenderer::createOutputPipelineLayout() {
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
			.setLayoutCount = 1,
			.pSetLayouts = &finalOutput.finalOutputDescriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};
		finalOutput.finalOutputPipelineLayout = vulkanCore.device.createPipelineLayout(pipelineLayoutInfo);
		createOutputPipeline();
	}

	void VulkanRenderer::createOutputPipeline() {
		vk::ShaderModule shaderModule = createShaderModule(readFile(SHADERDIR"/finalOut.slang.spv"));
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

		vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.width),
		static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.height), 0.0f, 1.0f };

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
		.pSetLayouts = &finalOutput.finalOutputDescriptorSetLayout };
		finalOutput.finalOutputPipelineLayout = vulkanCore.device.createPipelineLayout(pipelineLayoutInfo);
		vk::Format colorAttachmentFormat = static_cast<vk::Format>(vulkanCore.vkbInstances.swapChain.image_format);

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
		 .layout = finalOutput.finalOutputPipelineLayout,
		 .renderPass = nullptr},
		{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorAttachmentFormat} };

		auto pipelineResult = vulkanCore.device.createGraphicsPipeline(nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		if (pipelineResult.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create final output pipeline!");
		}
		finalOutput.finalOutputPipeline = pipelineResult.value;
		vulkanCore.device.destroyShaderModule(shaderModule);
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
		vk::SemaphoreCreateInfo semaphoreInfo{};

		for (auto &frame : vulkanCore.perFrame) {
			vk::FenceCreateInfo fenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };
			frame.renderFence = vulkanCore.device.createFence(fenceInfo);
			frame.presentSemaphore = vulkanCore.device.createSemaphore(semaphoreInfo);
		}
		for (size_t i = 0; i < vulkanCore.swapChainImages.size(); i++) {
			vulkanCore.renderSemaphores.push_back(vulkanCore.device.createSemaphore(semaphoreInfo));
		}
	}


	void VulkanRenderer::Update(float dt) {
		drawFrame();
	}

	void VulkanRenderer::updateCameraBuffer(uint32_t frameIndex) {
		glm::mat4 cameraView = gameworld.getCameraView();
		glm::mat4 cameraProjection = getProjMatrix();
		CameraInfo* cameraInfo = new CameraInfo{ .view = cameraView, .projection = cameraProjection };
		void* data;
		vmaMapMemory(vulkanCore.allocator, cameraBufferAllocations[frameIndex], &data);
		std::memcpy(data, cameraInfo, sizeof(CameraInfo));
		vmaUnmapMemory(vulkanCore.allocator, cameraBufferAllocations[frameIndex]);

		// Free the temporary CameraInfo returned by Gameworld
		delete cameraInfo;
		cameraInfo = nullptr;
	}

	struct PushConstants {
		uint32_t instanceBaseOffset;
	};

	

	void VulkanRenderer::drawFrame() {
		// Draw skybox first into its own image view
		// Then draw regular into another image view.
		//Then in a final pass combine the two where if alpha is 0 sample from the skybox.


		auto fenceResult = vulkanCore.device.waitForFences(vulkanCore.perFrame[currentFrame].renderFence, vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to wait for fence!");
		}
		auto [result, imageIndex] = vulkanCore.device.acquireNextImageKHR(
			vulkanCore.swapChain, UINT64_MAX, vulkanCore.perFrame[currentFrame].presentSemaphore, nullptr);
		vulkanCore.device.resetFences(vulkanCore.perFrame[currentFrame].renderFence);

		std::vector<RenderTransmition>* renderTransmissions = gameworld.getRenderTransmitions();
		std::vector<MeshInstanceBatch> meshInstanceBatches;
		void* mappedData;
		vmaMapMemory(vulkanCore.allocator, storageBufferAllocations[currentFrame], &mappedData);
		BuildInstanceBatches(*renderTransmissions, meshInstanceBatches, mappedData);
		vmaUnmapMemory(vulkanCore.allocator, storageBufferAllocations[currentFrame]);
		updateCameraBuffer(currentFrame);
		drawSkyboxPass(imageIndex);
		drawGBufferPass(imageIndex, meshInstanceBatches); // Fixed imageIndex being passed
		drawFinalOutputPass(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask = (vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo{ .waitSemaphoreCount = 1,
								  .pWaitSemaphores = &vulkanCore.perFrame[currentFrame].presentSemaphore,
								  .pWaitDstStageMask = &waitDestinationStageMask,
								  .commandBufferCount = 1,
								  .pCommandBuffers = &vulkanCore.commandBuffers[currentFrame],
								  .signalSemaphoreCount = 1,
								  .pSignalSemaphores = &vulkanCore.renderSemaphores[imageIndex]};

		vulkanCore.queues.graphicsQueue.submit(submitInfo, vulkanCore.perFrame[currentFrame].renderFence);

		const vk::PresentInfoKHR presentInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &vulkanCore.renderSemaphores[imageIndex],
			.swapchainCount = 1,
			.pSwapchains = &vulkanCore.swapChain,
			.pImageIndices = &imageIndex,
			.pResults = nullptr
		};
		
		result = static_cast<vk::Result>(
			VULKAN_HPP_DEFAULT_DISPATCHER.vkQueuePresentKHR(
				static_cast<VkQueue>(vulkanCore.queues.graphicsQueue),
				reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo)));

		if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR) || framebufferResized) {
			std::cout << "Swap chain is out of date or suboptimal, recreating swap chain..." << std::endl;
			framebufferResized = false;
			recreateSwapChain();
		}
		else {
			assert(result == vk::Result::eSuccess);
		}
		
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanRenderer::drawSkyboxPass(uint32_t imageIndex) {
		vk::CommandBuffer commandBuffer = vulkanCore.commandBuffers[currentFrame];
		vk::CommandBufferBeginInfo beginInfo{};
		commandBuffer.begin(beginInfo);
		uint32_t mipLevels = 1;
		transitionImageLayout(commandBuffer,
			skybox.skyboxRenderOutputImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor,
			mipLevels
		);
		int width = vulkanCore.vkbInstances.swapChain.extent.width;
		int height = vulkanCore.vkbInstances.swapChain.extent.height;
		vk::Extent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		vk::RenderPassBeginInfo renderPassInfo{
			.renderPass = nullptr,
			.framebuffer = nullptr,
			.renderArea = vk::Rect2D({0, 0}, extent),
			.clearValueCount = 0,
			.pClearValues = nullptr
		};

		vk::RenderingAttachmentInfo colorAttachment{
			.imageView = skybox.skyboxRenderOutputView,
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = { std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f} }
		};

		vk::RenderingInfo renderingInfo{
			.renderArea = vk::Rect2D({ 0, 0 }, extent),
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr
		};

		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.width), static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.height), 0.0f, 1.0f });
		commandBuffer.setScissor(0, vk::Rect2D({ 0, 0 }, extent));
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, skybox.skyboxPipeline);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skybox.skyboxPipelineLayout, 0, skybox.skyboxDescriptorSets[currentFrame], {});
		commandBuffer.bindVertexBuffers(0, quad.vertexBuffer, { 0 });
		commandBuffer.bindIndexBuffer(quad.indexBuffer, 0, vk::IndexType::eUint32);
		commandBuffer.drawIndexed(static_cast<uint32_t>(quad.indices.size()), 1, 0, 0, 0);
		commandBuffer.endRendering();
		transitionImageLayout(commandBuffer,
			skybox.skyboxRenderOutputImage,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::AccessFlagBits2::eColorAttachmentWrite,              // srcAccessMask
			vk::AccessFlagBits2::eShaderRead,                        // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,      // srcStage
			vk::PipelineStageFlagBits2::eFragmentShader,             // dstStage
			vk::ImageAspectFlagBits::eColor,
			1);
		 /*transitionImageLayout(commandBuffer,
			skybox.skyboxRenderOutputImage,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::AccessFlagBits2::eShaderRead,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eFragmentShader,
			vk::ImageAspectFlagBits::eColor,
			1
		 );*/

		//commandBuffer.end();
	}



	void VulkanRenderer::drawGBufferPass(uint32_t imageIndex, const std::vector<MeshInstanceBatch>& meshInstanceBatches) {
		// Use currentFrame, not frameIndex
		vk::CommandBuffer* commandBuffer = &vulkanCore.commandBuffers[currentFrame];
		//commandBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);

		vk::CommandBufferBeginInfo beginInfo{};
		//commandBuffer->begin(beginInfo);
		// We have to specify mipLevels, default is 1 since we're writing to the swapchain colour attachment directly here
		uint32_t mipLevels = 1;
		transitionImageLayout(*commandBuffer,
			gBuffer.gbufferImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor,
			mipLevels
		);
		transitionImageLayout(*commandBuffer,
			depthImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::ImageAspectFlagBits::eDepth,
			1
		);

		int width = vulkanCore.vkbInstances.swapChain.extent.width;
		int height = vulkanCore.vkbInstances.swapChain.extent.height;
		vk::Extent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		vk::RenderPassBeginInfo renderPassInfo{
			.renderPass = nullptr,
			.framebuffer = nullptr,
			.renderArea = vk::Rect2D({0, 0}, extent),
			.clearValueCount = 0,
			.pClearValues = nullptr
		};

		vk::RenderingAttachmentInfo colorAttachment{
			.imageView = gBuffer.gbufferImageView,
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = { std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f} }
		};

		vk::RenderingAttachmentInfo depthAttachment{
			.imageView = depthImageView,
			.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eDontCare,
			.clearValue = { vk::ClearDepthStencilValue{ 1.0f, 0 } }
		};

		vk::RenderingInfo renderingInfo{
			.renderArea = vk::Rect2D({ 0, 0 }, extent),
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = &depthAttachment,
			.pStencilAttachment = nullptr
		};
		commandBuffer->beginRendering(renderingInfo);
		commandBuffer->setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.width), static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.height), 0.0f, 1.0f });
		commandBuffer->setScissor(0, vk::Rect2D({ 0, 0 }, extent));
		commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, gBuffer.gBufferPipeline); // Fix typo: gBufferPipeline

		for (const auto& batch : meshInstanceBatches) {
			if (batch.instanceCount == 0) continue;

			// Push the instanceBaseOffset using push constants
			PushConstants pc{ .instanceBaseOffset = batch.ssboBaseOffset };
			commandBuffer->pushConstants(gBuffer.gbufferPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &pc);

			for (MeshComponent* piece : batch.pieces) {
				commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gBuffer.gbufferPipelineLayout, 0, piece->material.descriptorSets[currentFrame], {});

				commandBuffer->bindVertexBuffers(0, piece->vertexBuffer, { 0 });
				commandBuffer->bindIndexBuffer(piece->indexBuffer, 0, vk::IndexType::eUint32);
				commandBuffer->drawIndexed(static_cast<uint32_t>(piece->indices.size()), batch.instanceCount, 0, 0, 0);
			}
		}

		commandBuffer->endRendering();
		transitionImageLayout(*commandBuffer,
			gBuffer.gbufferImage,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::AccessFlagBits2::eColorAttachmentWrite,              // srcAccessMask
			vk::AccessFlagBits2::eShaderRead,                        // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,      // srcStage
			vk::PipelineStageFlagBits2::eFragmentShader,             // dstStage
			vk::ImageAspectFlagBits::eColor,
			1);
		//commandBuffer->end();
	}
	void VulkanRenderer::drawFinalOutputPass(uint32_t imageIndex) 
	{
		vk::CommandBuffer commandBuffer = vulkanCore.commandBuffers[currentFrame];
		transitionImageLayout(commandBuffer,
			vulkanCore.swapChainImages[imageIndex],
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor,
			1
		);

		int width = vulkanCore.vkbInstances.swapChain.extent.width;
		int height = vulkanCore.vkbInstances.swapChain.extent.height;
		vk::Extent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		vk::RenderingAttachmentInfo colorAttachment{
		.imageView = vulkanCore.swapChainImageViews[imageIndex],
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = { std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f} }
		};


		vk::RenderingInfo renderingInfo{
			.renderArea = vk::Rect2D({ 0, 0 }, extent),
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr
		};
		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.width), static_cast<float>(vulkanCore.vkbInstances.swapChain.extent.height), 0.0f, 1.0f });
		commandBuffer.setScissor(0, vk::Rect2D({ 0, 0 }, extent));
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, finalOutput.finalOutputPipeline);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, finalOutput.finalOutputPipelineLayout, 0, finalOutput.finalOutputDescriptorSets[currentFrame], {});
		commandBuffer.bindVertexBuffers(0, quad.vertexBuffer, { 0 });
		commandBuffer.bindIndexBuffer(quad.indexBuffer, 0, vk::IndexType::eUint32);
		commandBuffer.drawIndexed(static_cast<uint32_t>(quad.indices.size()), 1, 0, 0, 0);
		commandBuffer.endRendering();

		transitionImageLayout(commandBuffer,
			vulkanCore.swapChainImages[imageIndex],
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
			{},                                                     // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
			vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
			vk::ImageAspectFlagBits::eColor,
			1);
		commandBuffer.end();
	}

	void VulkanRenderer::BuildInstanceBatches(
		const std::vector<RenderTransmition>& renderables,
		std::vector<MeshInstanceBatch>& outBatches,
		void* ssboMapped)
	{
		// --- Group phase ---
		std::unordered_map<uint32_t, MeshInstanceBatch> batchMap;

		for (const RenderTransmition& renderable : renderables) {
			if (!renderable.mesh || renderable.mesh->empty()) continue;

			uint32_t meshKey = renderable.mesh->front().id;
			auto& batch = batchMap[meshKey];

			if (batch.pieces.empty()) {
				for (MeshComponent& piece : *renderable.mesh)
					batch.pieces.push_back(&piece);
			}

			batch.modelMatrices.push_back(renderable.modelMatrix);
		}

		// --- SSBO upload + offset assignment phase ---
		outBatches.clear();
		outBatches.reserve(batchMap.size());

		uint32_t cursor = 0;
		for (auto& [meshKey, batch] : batchMap) {
			batch.ssboBaseOffset = cursor;
			batch.instanceCount = static_cast<uint32_t>(batch.modelMatrices.size());

			memcpy(
				(glm::mat4*)ssboMapped + cursor,
				batch.modelMatrices.data(),
				batch.instanceCount * sizeof(glm::mat4)
			);

			cursor += batch.instanceCount;
			outBatches.push_back(std::move(batch));
		}
	}

}