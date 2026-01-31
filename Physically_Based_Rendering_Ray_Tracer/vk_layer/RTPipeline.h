#ifndef VULKAN_RAYTRACING_PIPELINE
#define VULKAN_RAYTRACING_PIPELINE

#include "base/base.h"
#include "Context.h"
#include "DescriptorManager.h"
#include "ShaderManager.h"

class RTPipeline {
public:
	RTPipeline(Context& context);

	void init();

	operator VkPipeline () const { return pipeline; }
	
	VkPipelineLayout pipeline_layout()const { return layout; }

	void create_pipeline(const pstd::vector<VkDescriptorSetLayout>& dsLayouts);

	~RTPipeline();

	

private:

	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;

	Context& _context;
};

#endif // !VULKAN_RAYTRACING_PIPELINE

