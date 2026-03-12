#include "ShaderModule.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <shaderc/shaderc.hpp>

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
	shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override {
		namespace fs = std::filesystem;

		fs::path requesting_dir = fs::path(requesting_source).parent_path();
		fs::path resolved = requesting_dir / requested_source;
		resolved = fs::canonical(resolved);

		std::ifstream file(resolved, std::ios::binary);
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		auto* result = new shaderc_include_result;
		auto* name = new std::string(resolved.string());
		auto* data = new std::string(std::move(content));

		result->source_name = name->c_str();
		result->source_name_length = name->size();
		result->content = data->c_str();
		result->content_length = data->size();
		result->user_data = new std::pair<std::string*, std::string*>(name, data);

		return result;
	}

	void ReleaseInclude(shaderc_include_result* data) override {
		auto* ptrs = static_cast<std::pair<std::string*, std::string*>*>(data->user_data);
		delete ptrs->first;
		delete ptrs->second;
		delete ptrs;
		delete data;
	}
};


static shaderc_shader_kind detect_shader_kind(const std::string& path) {
	if (path.find(".rgen") != std::string::npos) return shaderc_raygen_shader;
	if (path.find(".rmiss") != std::string::npos) return shaderc_miss_shader;
	if (path.find(".rchit") != std::string::npos) return shaderc_closesthit_shader;
	if (path.find(".rahit") != std::string::npos) return shaderc_anyhit_shader;
	if (path.find(".rint") != std::string::npos) return shaderc_intersection_shader;
	if (path.find(".comp") != std::string::npos) return shaderc_compute_shader;
	if (path.find(".vert") != std::string::npos) return shaderc_vertex_shader;
	if (path.find(".frag") != std::string::npos) return shaderc_fragment_shader;

	throw std::runtime_error("Pipeline::Unknown shader type: " + path);
}


ShaderModule::ShaderModule(VkDevice device, const std::string& shader_path) :_device(device) {
	// Compile Shaders
	std::ifstream file(shader_path, std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Pipeine::Failed to open shader file" + shader_path);

	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	shaderc_shader_kind kind = detect_shader_kind(shader_path);

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetTargetSpirv(shaderc_spirv_version_1_4);
	options.SetGenerateDebugInfo();
	options.SetOptimizationLevel(shaderc_optimization_level_zero);
	options.SetIncluder(std::make_unique<ShaderIncluder>());

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, kind, shader_path.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		throw std::runtime_error("ShaderModule::Shader comiilation failed! [" + shader_path + "]:\n" + result.GetErrorMessage());

	std::vector<uint32_t> spirv(result.cbegin(), result.cend());

	// Create Shader Modules

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	if (vkCreateShaderModule(_device, &createInfo, nullptr, &_shaderModule) != VK_SUCCESS)
		throw std::runtime_error("ShaderModule::Failed to create shader module for " + shader_path);
}




ShaderModule::ShaderModule(ShaderModule&& other) noexcept
	: _device(other._device), _shaderModule(other._shaderModule) {
	other._device = VK_NULL_HANDLE;
	other._shaderModule = VK_NULL_HANDLE;
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept {
	if (&other != this) {
		if (_shaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(_device, _shaderModule, nullptr);

		_shaderModule = other._shaderModule;
		_device = other._device;

		other._device = VK_NULL_HANDLE;
		other._shaderModule = VK_NULL_HANDLE;
	}
	return *this;
}

ShaderModule::~ShaderModule() {
	if (_shaderModule != VK_NULL_HANDLE)
		vkDestroyShaderModule(_device, _shaderModule, nullptr);
}