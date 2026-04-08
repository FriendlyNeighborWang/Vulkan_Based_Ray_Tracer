#include "Pipeline.h"

#include "Context.h"
#include "ShaderModule.h"
#include "VkMemoryAllocator.h"


Pipeline::Pipeline(VkDevice device, const std::string& name) :_device(device), pipeline_name(name){
	pipeline_id = next_pipeline_id++;
}

Pipeline::Pipeline(Pipeline&& other) noexcept: shaderModules(std::move(other.shaderModules)), descriptorSetLayouts(std::move(other.descriptorSetLayouts)), _pipeline(other._pipeline), _layout(other._layout), _device(other._device), pipeline_name(other.pipeline_name), pipeline_id(other.pipeline_id), pipeline_type(other.pipeline_type) {
	other._pipeline = VK_NULL_HANDLE;
	other._layout = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept{
	if (&other != this) {
		if (_pipeline != VK_NULL_HANDLE)vkDestroyPipeline(_device, _pipeline, nullptr);
		if (_layout != VK_NULL_HANDLE)vkDestroyPipelineLayout(_device, _layout, nullptr);
	}

	shaderModules = std::move(other.shaderModules);
	descriptorSetLayouts = std::move(other.descriptorSetLayouts);
	_pipeline = other._pipeline;
	_layout = other._layout;
	_device = other._device;
	pipeline_name = other.pipeline_name;
	pipeline_id = other.pipeline_id;
	pipeline_type = other.pipeline_type;

	other._pipeline = VK_NULL_HANDLE;
	other._layout = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;

	return *this;
}

Pipeline::~Pipeline() {
	if (_pipeline != VK_NULL_HANDLE)vkDestroyPipeline(_device, _pipeline, nullptr);
	if (_layout != VK_NULL_HANDLE)vkDestroyPipelineLayout(_device, _layout, nullptr);
}

void Pipeline::register_descriptor_set_layout(pstd::vector<VkDescriptorSetLayout>& layouts) {
	descriptorSetLayouts = std::move(layouts);
}

VkShaderModule Pipeline::register_shader(std::string shader_path) {
	ShaderModule shader_module(_device, shader_path);
	VkShaderModule vk_shader_module = shader_module;
	shaderModules.insert({ shader_path, std::move(shader_module)});

	return vk_shader_module;
}



RTPipeline::RTPipeline(Context& context, const std::string& name) :Pipeline(context, name) {
	pipeline_type = PipelineType::RAY_TRACING;

	if (vkCreateRayTracingPipelinesKHR != nullptr && vkGetRayTracingShaderGroupHandlesKHR != nullptr) return;
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_device, "vkCreateRayTracingPipelinesKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_device, "vkGetRayTracingShaderGroupHandlesKHR"));
}


RTPipeline::RTPipeline(RTPipeline&& other) noexcept : 
	Pipeline(std::move(other)),
	meta_group_infos(std::move(other.meta_group_infos)),
	stages(std::move(other.stages)),
	groups(std::move(other.groups)),
	sbtBuffer(std::move(other.sbtBuffer)),
	shaderHandleStorage(std::move(other.shaderHandleStorage)),
	shaderGroupRegion(std::move(other.shaderGroupRegion)){}


RTPipeline& RTPipeline::operator=(RTPipeline&& other) noexcept {
	if (&other != this) {
		Pipeline::operator=(std::move(other));
		meta_group_infos = std::move(other.meta_group_infos);
		stages = std::move(other.stages);
		groups = std::move(other.groups);
		sbtBuffer = std::move(other.sbtBuffer);
		shaderHandleStorage = std::move(other.shaderHandleStorage);
		shaderGroupRegion = std::move(other.shaderGroupRegion);
	}
	return *this;
}

void RTPipeline::register_raygen_shader(const std::string& path) {
	VkShaderModule shader_module = register_shader(path);

	// Shader Stage
	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	stage_info.module = shader_module;
	stage_info.pName = "main";
	stage_info.flags = 0;
	stage_info.pNext = nullptr;

	stages.push_back(stage_info);

	// Meta Shader Group
	ShaderGroup group;
	group.type = ShaderGroupType::RAY_GEN;
	group.stage_idx.push_back(stages.size() - 1);
	group.register_order = ShaderGroup::next_register_order++;

	meta_group_infos.push_back(group);
}

void RTPipeline::register_miss_shader(const std::string& path) {
	VkShaderModule shader_module = register_shader(path);

	// Shader Stage
	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	stage_info.module = shader_module;
	stage_info.pName = "main";
	stage_info.flags = 0;
	stage_info.pNext = nullptr;

	stages.push_back(stage_info);

	// Meta Shader Group
	ShaderGroup group;
	group.type = ShaderGroupType::MISS;
	group.stage_idx.push_back(stages.size() - 1);
	group.register_order = ShaderGroup::next_register_order++;

	meta_group_infos.push_back(group);
}

void RTPipeline::register_hit_group_shader(const std::string& closestHitPath, const std::string& anyHitPath, const std::string& intersectionPath) {
	VkShaderModule chit_module = VK_NULL_HANDLE;
	VkShaderModule ahit_module = VK_NULL_HANDLE;
	VkShaderModule int_module = VK_NULL_HANDLE;

	ShaderGroup group;
	group.type = ShaderGroupType::HIT;
	group.stage_idx.assign({ -1, -1, -1 });
	group.register_order = ShaderGroup::next_register_order++;

	if (!closestHitPath.empty()) {
		chit_module = register_shader(closestHitPath);
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		stageInfo.module = chit_module;
		stageInfo.pName = "main";
		stageInfo.flags = 0;
		stageInfo.pNext = nullptr;

		stages.push_back(stageInfo);

		group.stage_idx[0] = stages.size() - 1;
	}
	if (!anyHitPath.empty()) {
		ahit_module = register_shader(anyHitPath);
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		stageInfo.module = ahit_module;
		stageInfo.pName = "main";
		stageInfo.flags = 0;
		stageInfo.pNext = nullptr;

		stages.push_back(stageInfo);
		group.stage_idx[1] = stages.size() - 1;
	}
	if (!intersectionPath.empty()) {
		int_module = register_shader(intersectionPath);
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		stageInfo.module = int_module;
		stageInfo.pName = "main";
		stageInfo.flags = 0;
		stageInfo.pNext = nullptr;

		stages.push_back(stageInfo);
		group.stage_idx[2] = stages.size() - 1;
	}

	meta_group_infos.push_back(group);
}

VkShaderStageFlags RTPipeline::supported_shader_stages() const {
	return VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
}

void RTPipeline::build(Context& context, uint32_t push_constant_size) {
	arrange_shader_groups();

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
	rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProps{};
	deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps.pNext = &rtProps;
	vkGetPhysicalDeviceProperties2(context, &deviceProps);

	// Push Constant Range Setting
	assert(push_constant_size % 4 == 0, "Push constant's size must be a multiple of 4");
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	pushConstantRange.offset = 0;
	pushConstantRange.size = push_constant_size;

	// Create Pipeline Layout
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	layoutInfo.pSetLayouts = descriptorSetLayouts.data();
	if (push_constant_size > 0) {
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstantRange;
	}

	if (vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_layout) != VK_SUCCESS)
		throw std::runtime_error("RTPipeline::Failed to create pipeline layout");
	
	// Create Pipeline
	VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	pipelineInfo.pStages = stages.data();
	pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
	pipelineInfo.pGroups = groups.data();
	pipelineInfo.maxPipelineRayRecursionDepth = 2;
	pipelineInfo.layout = _layout;

	if (RTPipeline::vkCreateRayTracingPipelinesKHR(_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
		throw std::runtime_error("RTPipeline::Failed to create ray tracing pipeline");


	// Allocate Shader Binding Table
	const VkDeviceSize sbtHandleSize = rtProps.shaderGroupHandleSize;
	const VkDeviceSize sbtBaseAlignment = rtProps.shaderGroupBaseAlignment;
	const VkDeviceSize sbtHandleAlignment = rtProps.shaderGroupHandleAlignment;

	assert(sbtBaseAlignment % sbtHandleAlignment == 0);

	VkDeviceSize sbtStride = sbtBaseAlignment * ((sbtHandleSize + sbtBaseAlignment - 1) / sbtBaseAlignment);
	assert(sbtStride <= rtProps.maxShaderGroupStride);

	shaderHandleStorage.resize(sbtHandleSize * groups.size());
	if (RTPipeline::vkGetRayTracingShaderGroupHandlesKHR(
		_device,
		_pipeline,
		0, static_cast<uint32_t>(groups.size()),
		shaderHandleStorage.size(), shaderHandleStorage.data()
	) != VK_SUCCESS)
		throw std::runtime_error("RTPipeline::Failed to get shader group handles");

	uint32_t sbtSize = static_cast<uint32_t>(sbtStride * groups.size());
	sbtBuffer = context.memAllocator().create_buffer(
		sbtSize,
		sbtSize,
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	uint8_t* mappedSBT = reinterpret_cast<uint8_t*>(sbtBuffer.map_memory());
	for (size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
		memcpy(&mappedSBT[groupIndex * sbtStride], &shaderHandleStorage[groupIndex * sbtHandleSize], sbtHandleSize);
	}
	sbtBuffer.unmap_memory();

	shaderGroupRegion.resize(4);

	uint32_t raygenShaderCount = 0, missShaderCount = 0, hitGroupCount = 0;
	for (const auto& info : meta_group_infos) {
		switch (info.type) {
		case ShaderGroupType::RAY_GEN: raygenShaderCount++; break;
		case ShaderGroupType::MISS: missShaderCount++; break;
		case ShaderGroupType::HIT: hitGroupCount++; break;
		}
	}

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
	shaderGroupRegion[3].deviceAddress = 0;
	shaderGroupRegion[3].stride = 0;
	shaderGroupRegion[3].size = 0;

}

void RTPipeline::arrange_shader_groups() {
	// Sort shader groups Info
	std::sort(meta_group_infos.begin(), meta_group_infos.end());

	for (const auto& meta_group : meta_group_infos) {
		VkRayTracingShaderGroupCreateInfoKHR group{};
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		group.pNext = nullptr;
		
		switch (meta_group.type) {
		case ShaderGroupType::RAY_GEN:
			group.generalShader = static_cast<uint32_t>(meta_group.stage_idx[0]);
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			break;
		case ShaderGroupType::MISS:
			group.generalShader = static_cast<uint32_t>(meta_group.stage_idx[0]);
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			break;
		case ShaderGroupType::HIT:
			group.closestHitShader = meta_group.stage_idx[0] >= 0 ? meta_group.stage_idx[0] : VK_SHADER_UNUSED_KHR;
			group.anyHitShader = meta_group.stage_idx[1] >= 0 ? meta_group.stage_idx[1] : VK_SHADER_UNUSED_KHR;
			group.intersectionShader = meta_group.stage_idx[2] >= 0 ? meta_group.stage_idx[2] : VK_SHADER_UNUSED_KHR;
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			break;
		}

		groups.push_back(group);
	}
}





ComputePipeline::ComputePipeline(Context& context, const std::string& name) :Pipeline(context, name) {
	pipeline_type = PipelineType::COMPUTE;
}

ComputePipeline::ComputePipeline(ComputePipeline&& other) noexcept :Pipeline(std::move(other)) {}

ComputePipeline& ComputePipeline::operator=(ComputePipeline&& other) noexcept {
	if (&other != this) {
		Pipeline::operator=(std::move(other));
	}
	return *this;
}

void ComputePipeline::register_compute_shader(const std::string& path) {
	register_shader(path);
}

VkShaderStageFlags ComputePipeline::supported_shader_stages() const {
	return VK_SHADER_STAGE_COMPUTE_BIT;
}

void ComputePipeline::build(Context& context, uint32_t push_constant_size) {

	assert(push_constant_size % 4 == 0 && "Push constant size must be a multiple of 4");
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = push_constant_size;

	// Create Pipeline Layout
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	layoutInfo.pSetLayouts = descriptorSetLayouts.data();
	if (push_constant_size > 0) {
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstantRange;
	}

	if (vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_layout) != VK_SUCCESS)
		throw std::runtime_error("ComputePipeline::Failed to create compute pipeline layout");

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageCreateInfo.module = shaderModules.begin()->second;
	shaderStageCreateInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStageCreateInfo;
	pipelineInfo.layout = _layout;

	if (vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
		throw std::runtime_error("ComputePipeline::Failed to create compute pipeline");
}


GraphicsPipeline::GraphicsPipeline(Context& context, const std::string& name) :Pipeline(context, name) {
	pipeline_type = PipelineType::GRAPHICS;
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept:
	Pipeline(std::move(other)),
	stages(std::move(other.stages)),
	vertexInputBindings(std::move(other.vertexInputBindings)),
	vertexInputAttributes(std::move(other.vertexInputAttributes)),
	colorAttachments(std::move(other.colorAttachments)),
	colorAttachmentsFormats(std::move(other.colorAttachmentsFormats)),
	depthAttachmentFormat(std::move(other.depthAttachmentFormat)),
	stencilAttachmentFormat(std::move(other.stencilAttachmentFormat)),
	cull_mode(other.cull_mode){}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept {
	if (&other != this) {
		Pipeline::operator=(std::move(other));
		stages = std::move(other.stages);
		vertexInputBindings = std::move(other.vertexInputBindings);
		vertexInputAttributes = std::move(other.vertexInputAttributes);
		colorAttachments = std::move(other.colorAttachments);
		colorAttachmentsFormats = std::move(other.colorAttachmentsFormats);
		depthAttachmentFormat = std::move(other.depthAttachmentFormat);
		stencilAttachmentFormat = std::move(other.stencilAttachmentFormat);
		cull_mode = other.cull_mode;
	}

	return *this;
}

void GraphicsPipeline::register_vertex_shader(const std::string& path) {
	VkShaderModule shader_module = register_shader(path);

	// Shader Stage
	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	stage_info.module = shader_module;
	stage_info.pName = "main";
	stage_info.flags = 0;
	stage_info.pNext = nullptr;

	stages.push_back(stage_info);

}

void GraphicsPipeline::register_fragment_shader(const std::string& path) {
	VkShaderModule shader_module = register_shader(path);
	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stage_info.module = shader_module;
	stage_info.pName = "main";
	stage_info.flags = 0;
	stage_info.pNext = nullptr;

	stages.push_back(stage_info);
}

void GraphicsPipeline::register_vertex_input(const pstd::vector<VkVertexInputBindingDescription> bindings, const pstd::vector<VkVertexInputAttributeDescription>& attributes) {
	vertexInputBindings = bindings;
	vertexInputAttributes = attributes;
}

void GraphicsPipeline::register_color_attachment(VkFormat format, VkColorComponentFlags colorWriteMask, VkBool32 blendEnable) {
	VkPipelineColorBlendAttachmentState attachment{};
	attachment.colorWriteMask = colorWriteMask;
	attachment.blendEnable = blendEnable;

	// TODO:: if(blendEnable == VK_SUCCESS)

	colorAttachments.push_back(attachment);
	colorAttachmentsFormats.push_back(format);
}

void GraphicsPipeline::register_depth_stencil_attachment(VkFormat format) {
	bool hasDepth = false;
	bool hasStencil = false;

	switch (format){
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
		hasDepth = true;
		break;
	case VK_FORMAT_S8_UINT:
		hasStencil = true;
		break;
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		hasDepth = true;
		hasStencil = true;
		break;
	default:
		throw std::runtime_error("GraphicsPipeline::The depth/stencil format is not supported");
	}

	if (hasDepth) {
		if (!depthAttachmentFormat.empty())
			throw std::runtime_error("GraphicsPipeline::Depth attachment already registered");
		depthAttachmentFormat.push_back(format);
	}

	if (hasStencil) {
		if (!stencilAttachmentFormat.empty())
			throw std::runtime_error("GraphicsPipeline::Stencil attachment already registered");
		stencilAttachmentFormat.push_back(format);
	}
}

VkShaderStageFlags GraphicsPipeline::supported_shader_stages() const {
	return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
}

void GraphicsPipeline::build(Context&, uint32_t push_constant_size)  {
	// Pipeline Layout
	assert(push_constant_size % 4 == 0, "Push constant's size must be a multiple of 4");
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = push_constant_size;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	layoutInfo.pSetLayouts = descriptorSetLayouts.data();
	if (push_constant_size > 0) {
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstantRange;
	}

	if (vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_layout) != VK_SUCCESS)
		throw std::runtime_error("GraphicsPipeline::Failed to create pipeline layout");

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

	// Vertex Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport / Scissor (Dynamic)
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = cull_mode;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// MultiSample
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;

	// Depth / Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	if (!depthAttachmentFormat.empty()) {
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
	}
	depthStencil.stencilTestEnable = VK_FALSE;

	// Dynamic State
	pstd::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
	
	// ColorAttachment
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = static_cast<uint32_t>(colorAttachments.size());
	colorBlending.pAttachments = colorAttachments.data();

	
	// Dynamic Rendering Info
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
	renderingInfo.pColorAttachmentFormats = colorAttachmentsFormats.data();
	renderingInfo.depthAttachmentFormat = depthAttachmentFormat.empty() ? VK_FORMAT_UNDEFINED : depthAttachmentFormat.back();
	renderingInfo.stencilAttachmentFormat = stencilAttachmentFormat.empty() ? VK_FORMAT_UNDEFINED : stencilAttachmentFormat.back();


	// Create Pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &renderingInfo;
	pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = _layout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
		throw std::runtime_error("GraphicsPipeline::Failed to create Graphics Pipeline");
}
