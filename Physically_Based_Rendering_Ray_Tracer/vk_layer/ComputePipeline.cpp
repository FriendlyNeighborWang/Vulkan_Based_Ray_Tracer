#include "ComputePipeline.h"


ComputePipeline::ComputePipeline(Context& context):_context(context){}

ComputePipeline::~ComputePipeline() {
	if (pipeline != VK_NULL_HANDLE)vkDestroyPipeline(_context, pipeline, nullptr);
	if (layout != VK_NULL_HANDLE)vkDestroyPipelineLayout(_context, layout, nullptr);
}

void ComputePipeline::create_pipeline(const std::string& shaderPath, const pstd::vector<VkDescriptorSetLayout>& dsLayouts, uint32_t pushConstantsSize) {
	shaderModule = _context.shaderManager().create_shader_module(shaderPath);

	VkPushConstantRange pushConstant{};
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstant.offset = 0;
	pushConstant.size = pushConstantsSize;

	// Create Pipeline Layout
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32_t>(dsLayouts.size());
	layoutInfo.pSetLayouts = dsLayouts.data();
	if (pushConstantsSize > 0) {
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;
	}

	if (vkCreatePipelineLayout(_context, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
		throw std::runtime_error("ComputePipeline::Failed to create compute pipeline layout");

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageCreateInfo.module = shaderModule;
	shaderStageCreateInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStageCreateInfo;
	pipelineInfo.layout = layout;

	if (vkCreateComputePipelines(_context, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline)!= VK_SUCCESS)
		throw std::runtime_error("ComputePipeline::Failed to create compute pipeline");
}