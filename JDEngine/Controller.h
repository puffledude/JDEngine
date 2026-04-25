#pragma once
#include <glm/vec2.hpp>

namespace JD {

	class Controller {
	
	public:
		virtual glm::vec2 getLeftStickDir() = 0;
		virtual glm::vec2 getRightStickDir() = 0;	
	};

}
