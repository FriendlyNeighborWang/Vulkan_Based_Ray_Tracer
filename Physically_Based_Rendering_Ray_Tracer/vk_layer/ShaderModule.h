#ifndef VULKAN_SHADER_MODULE
#define VULKAN_SHADER_MODULE

#include "base/base.h"

class ShaderModule {
	friend class Pipeline;
public:
	ShaderModule() {}
	ShaderModule(ShaderModule&& other) noexcept;
	ShaderModule& operator=(ShaderModule&& other) noexcept;

	~ShaderModule();

	operator const VkShaderModule& () const { return _shaderModule; }

private:
	ShaderModule(VkDevice device, const std::string& shader_path);

	VkShaderModule _shaderModule = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
};


#endif // !VULKAN_SHADER_MODULE


