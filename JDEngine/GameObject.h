#include "RenderObject.h"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>


namespace JD {
	class GameObject
	{
	public:
		GameObject() = default;
		void AddRenderObject(BasicShape shape) {
			renderObj = new RenderObject(shape);
		}
		void AddRenderObject(std::string path) {
			renderObj = new RenderObject(path);
		}
		RenderObject* GetRenderObject() {
			return renderObj;
		}
		virtual void Update(float dt) = 0;
	
	protected:
		RenderObject* renderObj = nullptr;
		
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	};
};
