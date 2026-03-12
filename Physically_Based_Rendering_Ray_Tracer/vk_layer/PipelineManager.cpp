#include "PipelineManager.h"

PipelineManager::PipelineManager(Context& context):_context(context){}


RTPipeline& PipelineManager::create_rt_pipeline(const std::string& name) {
	std::unique_ptr ptr = std::make_unique<RTPipeline>(_context, name);
	RTPipeline& ref = *ptr;

	uint32_t index = static_cast<uint32_t>(pipelines.size());
	pipelines.push_back(std::move(ptr));
	ids.push_back(ref.get_id());
	names.push_back(name);
	name_to_index.insert({ name, index });
	id_to_index.insert({ ref.get_id(), index });

	return ref;
}

ComputePipeline& PipelineManager::create_compute_pipeline(const std::string& name) {
	std::unique_ptr ptr = std::make_unique<ComputePipeline>(_context, name);
	ComputePipeline& ref = *ptr;

	uint32_t index = static_cast<uint32_t>(pipelines.size());
	pipelines.push_back(std::move(ptr));
	ids.push_back(ref.get_id());
	names.push_back(name);
	name_to_index.insert({ name, index });
	id_to_index.insert({ ref.get_id(), index });

	return ref;
}


Pipeline& PipelineManager::get(const std::string& name) const { return *pipelines[name_to_index.at(name)]; }

Pipeline& PipelineManager::get(uint32_t id) const { return *pipelines[id_to_index.at(id)]; }

const std::string& PipelineManager::get_name(uint32_t id) const { return names[id_to_index.at(id)]; }
const uint32_t PipelineManager::get_id(std::string& name) const { return ids[name_to_index.at(name)]; }