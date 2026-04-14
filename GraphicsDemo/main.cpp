#include <iostream>
#include <chrono>
#include "VulkanRenderer.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
//plan is make window here then pass surface to the renderer.
//then loop while glfw true.



GLFWwindow* initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	return window;
	//glfwSetWindowUserPointer(window, this);
	//glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}


int main() {
	std::cout << "Hello, Graphics!" << std::endl;
	JD::Gameworld* world = new JD::Gameworld();
	JD::VulkanRenderer* renderer = new JD::VulkanRenderer(*world);
	GLFWwindow* window = initWindow();
	renderer->AssignWindow(window);
	auto lastTime = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
		lastTime = currentTime;
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	/*delete renderer;
	delete world;*/

	return 0;
}