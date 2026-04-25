#pragma once
#include "Controller.h"
#include <GLFW/glfw3.h>

namespace JD 
{
	class KeyboardController : public Controller
	{
			public:
				KeyboardController(GLFWwindow* window) : window(window) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				glm::vec2 getLeftStickDir() override;
				glm::vec2 getRightStickDir() override;

	protected:
		GLFWwindow* window;
		double lastCursorX = 0.0, lastCursorY = 0.0;

	};


}