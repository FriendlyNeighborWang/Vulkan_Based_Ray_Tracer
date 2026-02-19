#include "ShaderManager.h"

#include "util/FileOp.h"
#include "Context.h"
#include "VkMemoryAllocator.h"

ShaderModule::ShaderModule(VkDevice device, VkShaderModule shaderModule):_device(device), _shaderModule(shaderModule) {}


ShaderModule::ShaderModule(ShaderModule&& other) noexcept
	: _device(other._device), _shaderModule(other._shaderModule),
	type(other.type), hit_type(other.hit_type), idx(other.idx), hit_idx(other.hit_idx) {
	other._device = VK_NULL_HANDLE;
	other._shaderModule = VK_NULL_HANDLE;
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept {
	if (&other != this) {
		if (_shaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(_device, _shaderModule, nullptr);

		_shaderModule = other._shaderModule;
		_device = other._device;
		type = other.type;
		hit_type = other.hit_type;
		idx = other.idx;
		hit_idx = other.hit_idx;

		other._device = VK_NULL_HANDLE;
		other._shaderModule = VK_NULL_HANDLE;
	}
	return *this;
}

bool ShaderModule::operator<(const ShaderModule& other) const {
	if (type != other.type)
		return static_cast<int>(type) < static_cast<int>(other.type);
	else
		return idx < other.idx;
}

ShaderModule::~ShaderModule() {
	if (_shaderModule != VK_NULL_HANDLE)
		vkDestroyShaderModule(_device, _shaderModule, nullptr);
}




ShaderManager::ShaderManager(Context& context) :_context(context) {}

ShaderManager::~ShaderManager() {
	LOG_STREAM("ShaderManager") << "ShaderManager has been deconstructed" << std::endl;
}

void ShaderManager::init() {
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_context, "vkGetRayTracingShaderGroupHandlesKHR"));
}

void ShaderManager::auto_import_shader_file(const std::string& directory) {
	auto shader_spv_list = FileOp::findFilesWithPattern(directory, ".spv");
	
	for (const auto& spv : shader_spv_list) {
		if (spv.find(".rgen.") != std::string::npos)
			add_raygen_shader(spv);
		else if (spv.find(".rmiss.") != std::string::npos)
			add_miss_shader(spv);
		else if (spv.find(".rchit") != std::string::npos)
			add_hit_group_shader(spv);
	}
	
}

void ShaderManager::add_raygen_shader(const std::string& path) {
	ShaderModule shaderModule = create_shader_module(path);

	shaderModule.type = ShaderModule::ShaderType::RayGen;
	shaderModule.idx = raygenShaderCount++;

	modules.push_back(std::move(shaderModule));
	
}

void ShaderManager::add_miss_shader(const std::string& path) {
	ShaderModule shaderModule = create_shader_module(path);

	shaderModule.type = ShaderModule::ShaderType::Miss;
	shaderModule.idx = missShaderCount++;

	modules.push_back(std::move(shaderModule));
}

void ShaderManager::add_hit_group_shader(const std::string& closestHitPath, const std::string& anyHitPath, const std::string& intersectionPath) {
	ShaderModule chModule, ahModule, iModule;
	
	if (!closestHitPath.empty()) {
		chModule = create_shader_module(closestHitPath);
		chModule.type = ShaderModule::ShaderType::HitGroup;
		chModule.hit_type = ShaderModule::HitShaderType::ClosestHit;
		chModule.idx = hitShaderCount++;
		chModule.hit_idx = hitGroupCount;
		modules.push_back(std::move(chModule));
	}
	if (!anyHitPath.empty()) {
		ahModule = create_shader_module(anyHitPath);
		ahModule.type = ShaderModule::ShaderType::HitGroup;
		ahModule.hit_type = ShaderModule::HitShaderType::AnyHit;
		ahModule.idx = hitShaderCount++;
		ahModule.hit_idx = hitGroupCount;
		modules.push_back(std::move(ahModule));
	}
	if (!intersectionPath.empty()) {
		iModule = create_shader_module(intersectionPath);
		iModule.type = ShaderModule::ShaderType::HitGroup;
		iModule.hit_type = ShaderModule::HitShaderType::Intersection;
		iModule.idx = hitShaderCount++;
		iModule.hit_idx = hitGroupCount;
		modules.push_back(std::move(iModule));
	}

	++hitGroupCount;
}


void ShaderManager::build_shader_stages_and_shader_groups () {
	std::sort(modules.begin(), modules.end(), [](const ShaderModule& lhs, const ShaderModule& rhs) {return lhs < rhs; });

	// Shader Stage
	stages.resize(raygenShaderCount + missShaderCount + hitShaderCount);
	for (int i = 0; i < modules.size(); ++i) {

		VkShaderStageFlagBits stage;
		switch (modules[i].type) {
		case ShaderModule::ShaderType::RayGen:
			stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			break;
		case ShaderModule::ShaderType::Miss:
			stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			break;
		case ShaderModule::ShaderType::HitGroup:
			switch (modules[i].hit_type) {
			case ShaderModule::HitShaderType::ClosestHit:
				stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				break;
			case ShaderModule::HitShaderType::AnyHit:
				stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				break;
			case ShaderModule::HitShaderType::Intersection:
				stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				break;
			}
			break;
		}

		stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[i].stage = stage;
		stages[i].module = modules[i];
		stages[i].pName = "main";
		stages[i].flags = 0;
		stages[i].pNext = nullptr;
	}

	// Shader Group
	groups.resize(raygenShaderCount + missShaderCount + hitGroupCount);

	for (int m_i = 0, g_i = 0; m_i < modules.size(); ++m_i, ++g_i) {
		switch (modules[m_i].type) {
		case ShaderModule::ShaderType::RayGen:
		{
			groups[g_i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groups[g_i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groups[g_i].generalShader = m_i;
			groups[g_i].closestHitShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].anyHitShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].intersectionShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].pNext = nullptr;
		}
		break;
		case ShaderModule::ShaderType::Miss:
		{
			groups[g_i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groups[g_i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groups[g_i].generalShader = m_i;
			groups[g_i].closestHitShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].anyHitShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].intersectionShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].pNext = nullptr;
		}
		break;
		case ShaderModule::ShaderType::HitGroup:
		{
			const uint32_t hit_group_idx = modules[m_i].hit_idx;
			uint32_t ch_idx = VK_SHADER_UNUSED_KHR,
				ah_idx = VK_SHADER_UNUSED_KHR,
				i_idx = VK_SHADER_UNUSED_KHR;

			auto grouptype = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

			int step = -1;
			
			for (int j = 0; j < 3 && (m_i + j) < modules.size(); ++j) {
				if (modules[m_i + j].hit_idx != hit_group_idx)break;

				switch (modules[m_i + j].hit_type)
				{
				case ShaderModule::HitShaderType::ClosestHit:
					ch_idx = m_i + j;
					++step;
					break;
				case ShaderModule::HitShaderType::AnyHit:
					ah_idx = m_i + j;
					++step;
					break;
				case ShaderModule::HitShaderType::Intersection:
					i_idx = m_i + j;
					grouptype = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
					++step;
					break;
				}
			}
			m_i += step;

			groups[g_i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groups[g_i].type = grouptype;
			groups[g_i].generalShader = VK_SHADER_UNUSED_KHR;
			groups[g_i].closestHitShader = ch_idx;
			groups[g_i].anyHitShader = ah_idx;
			groups[g_i].intersectionShader = i_idx;
			groups[g_i].pNext = nullptr;
		}
		break;
		}
	}

}

void ShaderManager::build_shader_binding_table(VkPipeline rtPipeline) {
	// get SBT stride
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties{};
	rtPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 physicalDeviceProperties{};
	physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physicalDeviceProperties.pNext = &rtPipelineProperties;

	vkGetPhysicalDeviceProperties2(_context, &physicalDeviceProperties);

	const VkDeviceSize sbtHandleSize = rtPipelineProperties.shaderGroupHandleSize;
	const VkDeviceSize sbtBaseAlignment = rtPipelineProperties.shaderGroupBaseAlignment;
	const VkDeviceSize sbtHandleAlignment = rtPipelineProperties.shaderGroupHandleAlignment;

	assert(sbtBaseAlignment % sbtHandleAlignment == 0);

	VkDeviceSize sbtStride = sbtBaseAlignment * ((sbtHandleSize + sbtBaseAlignment - 1) / sbtBaseAlignment);

	assert(sbtStride <= rtPipelineProperties.maxShaderGroupStride);

	// get handle
	shaderHandleStorage.resize(sbtHandleSize * groups.size());
	if (vkGetRayTracingShaderGroupHandlesKHR(
		_context,
		rtPipeline,
		0,
		static_cast<uint32_t>(groups.size()),
		shaderHandleStorage.size(),
		shaderHandleStorage.data()
	) != VK_SUCCESS)
		throw std::runtime_error("ShaderManager::Failed to get shader group handles");

	const uint32_t sbtSize = static_cast<uint32_t>(sbtStride * groups.size());
	sbtBuffer = _context.memAllocator().create_buffer(
		sbtSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	uint8_t* mappedSBT = reinterpret_cast<uint8_t*>(sbtBuffer.map_memory());
	for (size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
		memcpy(&mappedSBT[groupIndex * sbtStride], &shaderHandleStorage[groupIndex * sbtHandleSize], sbtHandleSize);
	}
	sbtBuffer.unmap_memory();


	// point to SBT region
	shaderGroupRegion.resize(4);

	const VkDeviceAddress sbtStartAddress = sbtBuffer.device_address();
	// Ray Gen
	shaderGroupRegion[0].deviceAddress = sbtStartAddress;
	shaderGroupRegion[0].stride = sbtStride;
	shaderGroupRegion[0].size = sbtStride * raygenShaderCount;

	// Miss
	shaderGroupRegion[1].deviceAddress = sbtStartAddress + sbtStride * raygenShaderCount;
	shaderGroupRegion[1].stride = sbtStride;
	shaderGroupRegion[1].size = sbtStride * missShaderCount;

	// Hit
	shaderGroupRegion[2].deviceAddress = sbtStartAddress + sbtStride * (raygenShaderCount + missShaderCount);
	shaderGroupRegion[2].stride = sbtStride;
	shaderGroupRegion[2].size = sbtStride * hitGroupCount;

	// Callable
	shaderGroupRegion[3].deviceAddress = sbtStartAddress;
	shaderGroupRegion[3].stride = sbtStride;
	shaderGroupRegion[3].size = 0;
	 
}




ShaderModule ShaderManager::create_shader_module(const std::string& path) const{
	auto code = FileOp::readFile(path);
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_context, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		throw std::runtime_error("ShaderManager::Failed to create shader module");

	return ShaderModule(_context, shaderModule);
}