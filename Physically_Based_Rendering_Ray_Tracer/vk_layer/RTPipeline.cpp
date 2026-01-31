#include "RTPipeline.h"

RTPipeline::RTPipeline(Context& context) :_context(context) {}

void RTPipeline::init() {
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_context, "vkCreateRayTracingPipelinesKHR"));
}

void RTPipeline::create_pipeline(const pstd::vector<VkDescriptorSetLayout>& dsLayouts) {

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
	rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProps{};
	deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps.pNext = &rtProps;
	vkGetPhysicalDeviceProperties2(_context, &deviceProps);

	// Push Constant Range Setting
	static_assert(sizeof(PushConstants) % 4 == 0, "Push constant's size must be a multiple of 4");
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstants);

	// Create Pipeline Layout

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushConstantRange;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(dsLayouts.size());
	layoutInfo.pSetLayouts = dsLayouts.data();

	if (vkCreatePipelineLayout(_context, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
		throw std::runtime_error("RTPipeline::Failed to create pipeline layout");

	// Create Pipeline
	const auto& stages = _context.shaderManager().get_stages();
	const auto& groups = _context.shaderManager().get_groups();

	VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	pipelineInfo.pStages = stages.data();
	pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
	pipelineInfo.pGroups = groups.data();
	pipelineInfo.maxPipelineRayRecursionDepth = std::min(5u, rtProps.maxRayRecursionDepth);
	pipelineInfo.layout = layout;

	if (vkCreateRayTracingPipelinesKHR(
		_context,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&pipeline
	) != VK_SUCCESS)
		throw std::runtime_error("RTPipeline::Failed to create ray tracing pipeline");
}

RTPipeline::~RTPipeline() {
	if (pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(_context, pipeline, nullptr);
	if (layout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(_context, layout, nullptr);

	LOG_STREAM("RTPipeline") << "RTPipeline has been deconstructed" << std::endl;
}