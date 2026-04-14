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
#include <stdexcept>

namespace JD
{
	class VulkanRenderer : Renderer {
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

		void initVulkan();
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createSwapChain();
		void createImageViews();
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDepthResources();
		void createGraphicsPipeline();
		void createCommandPool();
		void createTextureSampler();
		//void createUniformBuffers();
		//void createDescriptorSets();
		void createCommandBuffers();
		void createSyncObjects();
		
		
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



	};
};
