#include <vulkan/vulkan.hpp>
namespace JD
{
	struct RequiredPhysicalFeatures {

		vk::PhysicalDeviceFeatures2 features{
			.features = {.samplerAnisotropy = VK_TRUE},
		};

		vk::PhysicalDeviceVulkan11Features vulkan11Features{
		.shaderDrawParameters = VK_TRUE
		};
		vk::PhysicalDeviceVulkan13Features vulkan13Features{
			.dynamicRendering = VK_TRUE,
		};
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{
			.extendedDynamicState = VK_TRUE
		};

	};
}
