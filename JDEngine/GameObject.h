#include "RenderObject.h"

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
		virtual void Update(float dt) = 0;
	
	protected:
		RenderObject* renderObj = nullptr;
	
	};
};
