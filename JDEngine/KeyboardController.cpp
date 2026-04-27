#include "KeyboardController.h"
#include <glm/glm.hpp>
#include <glm/vec2.hpp>

namespace JD 
{
	glm::vec2 KeyboardController::getLeftStickDir(){
		glm::vec2 dir(0.0f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir.y += 1.0f;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir.y -= 1.0f;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir.x -= 1.0f;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir.x += 1.0f;
		if (glm::length(dir) > 0.0f) {
			dir = glm::normalize(dir);

		}
		return dir;
	}

	glm::vec2 KeyboardController::getRightStickDir() {
		double cursorX, cursorY;
		glfwGetCursorPos(window, &cursorX, &cursorY);
		glm::vec2 dir(cursorX - lastCursorX, cursorY - lastCursorY);
		lastCursorX = cursorX;
		lastCursorY = cursorY;
	/*	if (glm::length(dir) > 0.0f){
			dir = glm::normalize(dir);

		}*/
		return dir;
	}



}