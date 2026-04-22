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

		void initVulkan();
		void createDevices();
		void createSwapChain();
		void initVMA();
		vk::ImageView createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
		void createImageViews();
		void transitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
		vk::CommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(vk::CommandBuffer& commandBuffer);
		void copyBuffer(const vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);
		uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
		void createTextureImage(vk::Image image, std::string filePath);
		void createVertexBuffer(std::vector<Vertex>& verticies, vk::Buffer& buffer);
		void createIndexBuffer(std::vector<uint32_t>& indicies, vk::Buffer& buffer);
		void createDescriptorSetLayouts();
		void createDescriptorPool();
		void createDepthResources();
		void createGbufferDescriptorSetLayout();
		void createDescriptorSets();
		void createGbufferDescriptorSets();
		void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, VmaAllocation& allocation);
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& allocation);

		void copyBufferToImage(const vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height);
		void generateMipmaps(vk::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		void createGraphicsPipelines();
		void createGBufferPipeline();
		void createShadowPipeline();
		void createOutputPipeline();
		void createQueues();
		void createAlbedoPipeline();
		void createLightingPipeline();
		void createFinalImagePipeline();
		void createCommandPool();
		void createCommandBuffers();
		void createSyncObjects();
		void updateUniformBuffers();
		void updatePushConstants();

		void drawFrame();

		
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
		VulkanCore vulkanCore;
		GLFWwindow* window;
		uint32_t latestMeshID = 0;

		std::vector<vk::Buffer> storageBuffers;
		std::vector<VmaAllocation> storageBufferAllocations;
		uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		uint32_t currentFrame = 0;
		uint32_t MAX_OBJECTS = 1000;
		vk::DescriptorSetLayout gbufferDescriptorSetLayout;
		std::vector<vk::DescriptorSet> gbufferDescriptorSets;
		vk::PipelineLayout gbufferPipelineLayout;
		vk::Pipeline gBufferPipeline;
		vk::Image depthImage;
		vk::Format depthImageFormat;
		vk::ImageView depthImageView;
		VmaAllocation depthImageAllocation;

		vk::DescriptorPool descriptorPool;

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
