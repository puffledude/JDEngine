#pragma once
#include <stdexcept>
#include <cstdlib>
#include "Renderer.h"
#include "GameWorld.h"
#include "VulkanCore.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VkBootstrap.h"
#include <iostream>
#include "stb_image.h"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fstream>
#include <stdexcept>

namespace JD
{

	class VulkanRenderer : public Renderer {
	public:
		VulkanRenderer(Gameworld& gameworld);
		void AssignWindow(GLFWwindow* window);
		void Update(float dt) override;
		void loadGLTF(std::vector<MeshComponent>& meshComponents, std::string filePath) override;
		void wait() override {
			vulkanCore.device.waitIdle();
		}
		~VulkanRenderer();

	protected:
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
			auto* app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
		}
#
		// Helper function to create a Vulkan surface using GLFW
		// Taken from vk bootstrap example code: https://github.com/charles-lunarg/vk-bootstrap/blob/main/example/triangle.cpp
		VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow* window, VkAllocationCallbacks* allocator = nullptr) {
			VkSurfaceKHR surface = VK_NULL_HANDLE;
			VkResult err = glfwCreateWindowSurface(instance, window, allocator, &surface);
			if (err) {
				const char* error_msg;
				int ret = glfwGetError(&error_msg);
				if (ret != 0) {
					std::cout << ret << " ";
					if (error_msg != nullptr) std::cout << error_msg;
					std::cout << "\n";
				}
				surface = VK_NULL_HANDLE;
			}
			return surface;
		}

		float linearize(float c) {
			return c <= 0.04045f
				? c / 12.92f
				: powf((c + 0.055f) / 1.055f, 2.4f);
		}


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


		[[nodiscard]] vk::ShaderModule createShaderModule(const std::vector<char>& code) const
		{
			vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
			vk::ShaderModule shaderModule;
			vulkanCore.device.createShaderModule(&createInfo, nullptr, &shaderModule);
			return shaderModule;
		}
		glm::mat4& getProjMatrix() const {
			static glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)vulkanCore.vkbInstances.swapChain.extent.width / (float)vulkanCore.vkbInstances.swapChain.extent.height, 0.1f, 100.0f);
				//proj[1][1] *= -1; // Invert Y coordinate for Vulkan
				return proj;
		}

		void initVulkan();
		void createDevices();
		void createSwapChain();
		void createQueues();
		void initVMA();
		vk::ImageView createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, vk::ImageViewType viewType = vk::ImageViewType::e2D, uint32_t layerCount = 1);
		void createImageViews();
		void transitionImageLayout(vk::CommandBuffer& commandbuffer ,vk::Image image,
			vk::ImageLayout         old_layout,
			vk::ImageLayout         new_layout,
			vk::AccessFlags2        src_access_mask,
			vk::AccessFlags2        dst_access_mask,
			vk::PipelineStageFlags2 src_stage_mask,
			vk::PipelineStageFlags2 dst_stage_mask,
			vk::ImageAspectFlags image_aspect_flags,
			uint32_t mip_levels,
			uint32_t layer_count=1);
		vk::CommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(vk::CommandBuffer& commandBuffer);
		void copyBuffer(const vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);
		uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);


		//void createTextureImage(vk::Image image, std::string filePath);
		void createVkImageFromGLTFImage(vk::Image& image, vk::ImageView& imageView ,VmaAllocation& allocation, tinygltf::Image& gltfImage, vk::Format format);
		void createVertexBuffer(std::vector<Vertex>& verticies, vk::Buffer& buffer, VmaAllocation& allocation);
		void createIndexBuffer(std::vector<uint32_t>& indicies, vk::Buffer& buffer, VmaAllocation& allocation);
		void createTextureSampler();
		void createQuad();
		void createDescriptorSetLayouts();
		void createDescriptorPool();
		void createDepthResources();
		void createObjectDescriptorSetLayouts();
		

		void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, VmaAllocation& allocation);
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& allocation);

		void copyBufferToImage(const vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void generateMipmaps(vk::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount = 1);

		void createCameraBuffers();
		void createSunBuffers();
		
		
		void createGraphicsPipelines();
		void createGBufferPipeline();
		void createGBufferImages();


		void createCubeMapTextureImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& allocation);
		void loadCubemap(std::vector<std::string> faces, vk::Image& cubemapImage, VmaAllocation& cubemapAllocation, vk::ImageView& cubemapImageView);
		void loadSkybox();
		void createSkyboxDescriptorSetLayout();
		void createSkyboxPipeline();



		void createShadowDescriptorSetLayout();
		void createShadowDescriptorSets();
		void createShadowPipeline();

		void createLightingDescriptorSetLayout();
		void createLightingDescriptorSets();
		void createLightingPipeline();


		void createOutputDescriptorSetLayout();
		void createOutputDescriptorSets();
		void createOutputPipeline();



		void createCommandPool();
		void createCommandBuffers();
		void createSyncObjects();
		void DestroyMesh(std::vector<MeshComponent>& mesh) override;
		void DestroyAllRenderables();

		void recreateSwapChain();
		void sendRenderTransmition(std::vector<RenderTransmition>* renderTransmition) {
			currentRenderTransmition = renderTransmition;
		}
		void updateCameraBuffer(uint32_t frameIndex);
		void updateSunData(uint32_t frameIndex);
		void drawFrame();
		void drawShadowPass(const std::vector<MeshInstanceBatch>& meshInstanceBatches);
		void drawSkyboxPass();
		void drawGBufferPass(const std::vector<MeshInstanceBatch>& meshInstanceBatches);
		void drawLightPass(std::vector<lightTransmition>* lightTransmitions);
		void drawFinalOutputPass(uint32_t imageIndex);

		//void recordCommandBuffer(uint32_t imageIndex, const std::vector<MeshInstanceBatch>& meshInstanceBatches);
		void BuildInstanceBatches(
			const std::vector<RenderTransmition>& renderables,
			std::vector<MeshInstanceBatch>& outBatches,
			void* ssboMapped);
		
		void updateLightBuffer(uint32_t frameIndex, std::vector<lightTransmition>* lightTransmitions);


		void cleanupVulkan();
		void cleanupSwapChain();

		
		//tinygltf::Scene* makeGLTFScene(GLTFData data) override;
	/*	void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDepthResources();
		void createTextureSampler();
		void createUniformBuffers();
		void createDescriptorSets();
		void createCommandBuffers();
		void createSyncObjects();*/
		
		
		bool framebufferResized = false;
		uint32_t useTaa = true;
		float cooldown = 0.0f;


		VulkanCore vulkanCore;
		GLFWwindow* window;
		uint32_t latestMeshID = 0;

		std::vector<vk::Buffer> storageBuffers;
		std::vector<VmaAllocation> storageBufferAllocations;
		//uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		uint32_t currentFrame = 0;
		uint32_t MAX_OBJECTS = 1000;
		vk::DescriptorSetLayout objectDescriptorSetLayout;
		//std::vector<vk::DescriptorSet> gbufferDescriptorSets;

		std::vector<vk::Buffer> cameraBuffers;
		std::vector<VmaAllocation> cameraBufferAllocations;

		std::vector<vk::Buffer> sunBuffers;
		std::vector<VmaAllocation> sunBufferAllocations;

		vk::Image depthImage;
		vk::Format depthImageFormat;
		vk::ImageView depthImageView;
		VmaAllocation depthImageAllocation;

		vk::DescriptorPool descriptorPool;
		Skybox skybox{};
		Shadows shadows{};
		GBuffer gBuffer{};
		Lighting lighting{};
		CombineStep combineStep{};

		FinalOutput finalOutput{};
		std::vector<RenderTransmition>* currentRenderTransmition = nullptr;
		uint32_t jitterIndex = 0;
		MeshComponent quad;
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
		const std::vector<char const*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		int frameIndex = 0;

	};
};
