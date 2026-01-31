#ifndef VULKAN_COMPUTE_PIPELINE
#define VULKAN_COMPUTE_PIPELINE

#include "base/base.h"
#include "Context.h"

class ComputePipeline {
public:
	ComputePipeline(Context& context);
	~ComputePipeline();

	void init(){}

	operator VkPipeline() const { return pipeline; }

	VkPipelineLayout pipeline_layout() const { return layout; }

	void create_pipeline(const std::string& shaderPath, const pstd::vector<VkDescriptorSetLayout>& dsLayouts, uint32_t pushConstantsSize = 0);

private:
	ShaderModule shaderModule;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;

	Context& _context;

};


#endif // !VULKAN_COMPUTE_PIPELINE
