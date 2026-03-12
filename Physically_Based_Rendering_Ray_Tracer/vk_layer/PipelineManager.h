#ifndef VULKAN_PIPELINE_MANAGER
#define VULKAN_PIPELINE_MANAGER

#include "base/base.h"

#include "Pipeline.h"

class PipelineManager {
public:
	PipelineManager(Context& context);

	void init(){}

	RTPipeline& create_rt_pipeline(const std::string& name);
	ComputePipeline& create_compute_pipeline(const std::string& name);

	Pipeline& get(const std::string& name) const;
	Pipeline& get(uint32_t id) const;

	const pstd::vector<uint32_t>& all_ids() const { return ids; }
	const pstd::vector<std::string>& all_names() const { return names; }

	const std::string& get_name(uint32_t id) const;
	const uint32_t get_id(std::string& name) const;

private:
	pstd::vector<std::unique_ptr<Pipeline>> pipelines;

	pstd::vector<uint32_t> ids;
	pstd::vector<std::string> names;

	std::unordered_map<std::string, uint32_t> name_to_index;
	std::unordered_map<uint32_t, uint32_t> id_to_index;

	Context& _context;
};

#endif // !VULKAN_PIPELINE_MANAGER

