#ifndef PBRT_GRAPHICS_RESOURCE_MANAGER_H
#define PBRT_GRAPHICS_RESOURCE_MANAGER_H

#include "base/base.h"
#include "util/pstd.h"

#include <variant>
#include <unordered_set>

class ResourceManager {
	using ResourcePtr = std::variant<Buffer*, Image*, TLAS*, pstd::vector<Texture>*>;
public:
	ResourceManager(Context& context);
	~ResourceManager();

	// Register Resources
	void register_resources(Buffer&& buffer, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages);

	void register_resources(Image&& image, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages, VkImageLayout targetLayout);

	void register_resources(TLAS&& tlas, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages);

	void register_resources(pstd::vector<Texture>&& textures, pstd::vector<Sampler>&& samplers, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages);

	// For creating Image inside the function
	void register_resources(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages, VkImageLayout targetLayout);
	
	// For creating Buffer inside the function
	void register_resources(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages);

	
	void build(const std::string& header_file_name = "generated_descriptor_bindings.h");
	 
	void rebuild_window_size_related_resources(VkExtent2D extent);

	pstd::vector<VkDescriptorSet> get_descriptor_sets(uint32_t pipeline_id, uint32_t frame_idx);

	template<typename ReturnType>
	ReturnType* get_resource(const std::string& name) {
		auto it = resources_searching_list.find(name);
		if (it == resources_searching_list.end()) return nullptr;

		auto ptr = std::get_if<ReturnType*>(&it->second);
		return ptr ? *ptr : nullptr;
	}

private:
	std::string rf_flag_to_string(ResourceFlag flags);
	ResourceFlag rf_get_frequency(ResourceFlag flags);
	ResourceFlag rf_get_pipeline(uint32_t pipeline_id);

	void generate_header(const std::string& filename);

	struct DescriptorHandle {
		ResourceFlag flags;
		uint32_t binding;
		uint32_t idx;
		VkDescriptorType type;
		ResourcePtr ptr;
	};

	// Searching List & Cache
	std::unordered_map<std::string, ResourcePtr> resources_searching_list;
	pstd::vector<DescriptorHandle> window_size_related_resources_caches;

	// Descriptor Set
	std::unordered_map<ResourceFlag, DescriptorSetLayout*> layouts;  
	std::unordered_map<std::string, DescriptorSet*> sets;

	// Pipeline
	std::unordered_set<uint32_t> pipeline_ids;
	

	// Build Info
	pstd::vector<DescriptorHandle> descriptors;
	std::unordered_map<ResourceFlag, pstd::vector<std::string>> layout_binding_lists;

	// Meta Resources
	std::list<Image> images;
	std::list<Buffer> buffers;
	std::list<TLAS> tlas_list;
	std::list<pstd::vector<Texture>> textures_lists;
	std::list<pstd::vector<Sampler>> samplers_lists;

	// Context
	Context& _context;
};

#endif // !PBRT_GRAPHICS_RESOURCE_MANAGER_H

