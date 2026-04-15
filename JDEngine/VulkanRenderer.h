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
#include <stdexcept>

namespace JD
{

	class VulkanRenderer : public Renderer {
	public:
		VulkanRenderer(Gameworld& gameworld);
		void AssignWindow(GLFWwindow* window);
		void Update(float dt) override;

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


		void initVulkan();
		void createDevices();
		void createSwapChain();
		void initVMA();
		vk::ImageView createImageView(const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
		void createImageViews();
		void transitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
		vk::CommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(vk::CommandBuffer& commandBuffer);

		void createGraphicsPipelines();
		void createQueues();
		void createAlbedoPipeline();
		void createLightingPipeline();
		void createFinalImagePipeline();
		void createCommandPool();
		void createSyncObjects();
		void updateUniformBuffers();
		void updatePushConstants();

		void drawFrame();

		
		void cleanupVulkan();
		void cleanupSwapChain();
		tinygltf::Scene* makeGLTFScene(GLTFData data) override;
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
