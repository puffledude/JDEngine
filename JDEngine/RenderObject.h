#include <iostream>

namespace JD {
	
	enum class BasicShape {
		Quad,
		Triangle,
	};
	
	class RenderObject
	{
	public:
		//Take either an enum of a basic shape or filepath to a model
		RenderObject(std::string path);
		RenderObject(BasicShape shape);
		virtual void Update(float dt);
	};
};
