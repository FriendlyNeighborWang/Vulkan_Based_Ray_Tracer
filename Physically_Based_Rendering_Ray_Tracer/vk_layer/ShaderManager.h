#ifndef VULKAN_SHADER_MANAGER
#define VULKAN_SHADER_MANAGER

#include "base/base.h"
#include "util/FileOp.h"
#include "Buffer.h"
#include "VkMemoryAllocator.h"

#include <string>
#include <algorithm>

class ShaderModule {
	friend class ShaderManager;
public:
	ShaderModule(){}
	ShaderModule(ShaderModule&& other) noexcept;
	ShaderModule& operator=(ShaderModule&& other) noexcept;

	~ShaderModule();

	operator const VkShaderModule&() const{ return _shaderModule; }
	bool operator<(const ShaderModule& other)const;

	enum class ShaderType {
		None = -1, RayGen = 0, Miss = 1, HitGroup = 2
	};

	enum class HitShaderType {
		None = -1, ClosestHit = 0, AnyHit = 1, Intersection = 2
	};

	ShaderType type = ShaderType::None;
	HitShaderType hit_type = HitShaderType::None;
	uint32_t idx = 0;
	uint32_t hit_idx = 0;

private:
	ShaderModule(VkDevice device, VkShaderModule shaderModule);

	VkShaderModule _shaderModule = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
};

class ShaderManager {
public:
	ShaderManager(Context& context);
	~ShaderManager();

	void init();

	ShaderModule create_shader_module(const std::string& path) const;

	void auto_import_shader_file(const std::string& directory);

	void add_raygen_shader(const std::string& path);
	void add_miss_shader(const std::string& path);
	void add_hit_group_shader(const std::string& closestHitPath, const std::string& anyHitPath = "", const std::string& intersectionPath = "");
	
	void build_shader_stages_and_shader_groups();

	void build_shader_binding_table(VkPipeline rtPipeline);

	// Ray Gen Region, Miss Region, Hit Region, Callable Region
	const auto& get_shader_group_regions() { return shaderGroupRegion; }

	const auto& get_stages() { return stages; }
	const auto& get_groups() { return groups; }
private:

	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;

	pstd::vector<uint8_t> shaderHandleStorage;

	pstd::vector<VkStridedDeviceAddressRegionKHR> shaderGroupRegion;
	pstd::vector<VkPipelineShaderStageCreateInfo> stages;
	pstd::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
	pstd::vector<ShaderModule> modules;

	uint32_t raygenShaderCount = 0;
	uint32_t missShaderCount = 0;
	uint32_t hitShaderCount = 0;
	uint32_t hitGroupCount = 0;

	Buffer sbtBuffer;

	Context& _context;
};

#endif // !VULKAN_SHADER_MANAGER
