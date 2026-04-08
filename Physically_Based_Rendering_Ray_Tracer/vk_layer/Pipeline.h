#ifndef VULKAN_PIPELINE
#define VULKAN_PIPELINE

#include "base/base.h"
#include "util/pstd.h"
#include "Buffer.h"



class Pipeline {
public:
	enum class PipelineType { GRAPHICS, RAY_TRACING, COMPUTE};

	Pipeline(VkDevice device, const std::string& name);

	Pipeline(Pipeline&& other) noexcept;
	Pipeline& operator=(Pipeline&& other) noexcept;

	operator VkPipeline() const { return _pipeline; }
	VkPipelineLayout pipeline_layout() const { return _layout; }

	uint32_t get_id() const { return pipeline_id; }
	const std::string& get_name() const { return pipeline_name; }
	PipelineType get_pipeline_type() const { return pipeline_type; }

	void register_descriptor_set_layout(pstd::vector<VkDescriptorSetLayout>& layouts);

	virtual VkShaderStageFlags supported_shader_stages() const = 0;
	
	virtual void build(Context& context, uint32_t push_constant_size) = 0;

	virtual ~Pipeline();

protected:
	VkShaderModule register_shader(std::string shader_path);


	std::unordered_map <std::string, ShaderModule> shaderModules;
	pstd::vector<VkDescriptorSetLayout> descriptorSetLayouts;


	std::string pipeline_name;
	uint32_t pipeline_id;
	PipelineType pipeline_type;

	static inline uint32_t next_pipeline_id = 0;


	VkPipeline _pipeline = VK_NULL_HANDLE;
	VkPipelineLayout _layout = VK_NULL_HANDLE;

	VkDevice _device;

};



class RTPipeline: public Pipeline {
public:
	RTPipeline(Context& context, const std::string& name);
	RTPipeline(RTPipeline&& other) noexcept;
	RTPipeline& operator=(RTPipeline&& other) noexcept;
	
	void register_raygen_shader(const std::string& path);
	void register_miss_shader(const std::string& path);
	void register_hit_group_shader(const std::string& closestHitPath, const std::string& anyHitPath = "", const std::string& intersectionPath = "");

	virtual VkShaderStageFlags supported_shader_stages() const;

	virtual void build(Context& context, uint32_t push_constant_size) override;

	const auto& get_shader_group_regions() { return shaderGroupRegion; }

private:
	void arrange_shader_groups();

	enum class ShaderGroupType { RAY_GEN, MISS, HIT };
	struct ShaderGroup {
		ShaderGroupType type;
		pstd::vector<int32_t> stage_idx;
		uint32_t register_order;
		static inline uint32_t next_register_order = 0;

		bool operator<(const ShaderGroup& other) const {
			if (type == other.type) {
				return register_order < other.register_order;
			}
			else {
				return static_cast<uint32_t>(type) < static_cast<uint32_t>(other.type);
			}	
		}
	};

	pstd::vector<ShaderGroup> meta_group_infos;

	pstd::vector<VkPipelineShaderStageCreateInfo> stages;
	pstd::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

	Buffer sbtBuffer;
	pstd::vector<uint8_t> shaderHandleStorage;
	pstd::vector<VkStridedDeviceAddressRegionKHR> shaderGroupRegion;

	static inline PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
	static inline PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
};


class ComputePipeline : public Pipeline{
public:
	ComputePipeline(Context& context, const std::string& name);
	ComputePipeline(ComputePipeline&& other) noexcept;
	ComputePipeline& operator=(ComputePipeline&& other) noexcept;


	void register_compute_shader(const std::string& path);

	virtual VkShaderStageFlags supported_shader_stages() const;

	virtual void build(Context& context, uint32_t push_constant_size) override;

};


class GraphicsPipeline : public Pipeline {
public:
	GraphicsPipeline(Context& context, const std::string& name);
	GraphicsPipeline(GraphicsPipeline&& other) noexcept;
	GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

	// Shader
	void register_vertex_shader(const std::string& path);
	void register_fragment_shader(const std::string& path);

	// Vertex Input
	void register_vertex_input(const pstd::vector<VkVertexInputBindingDescription> bindings, const pstd::vector<VkVertexInputAttributeDescription>& attributes);

	// Rasterization Setting
	void set_rasterization_cull_mode(VkCullModeFlags mode) { cull_mode = mode; }

	void register_color_attachment(VkFormat format, VkColorComponentFlags colorWriteMask, VkBool32 blendEnable);
	void register_depth_stencil_attachment(VkFormat format);

	virtual VkShaderStageFlags supported_shader_stages() const;

	virtual void build(Context&, uint32_t push_constant_size) override;
	
private:
	pstd::vector<VkPipelineShaderStageCreateInfo> stages;

	pstd::vector<VkVertexInputBindingDescription> vertexInputBindings;
	pstd::vector<VkVertexInputAttributeDescription> vertexInputAttributes;

	pstd::vector<VkPipelineColorBlendAttachmentState> colorAttachments;
	pstd::vector<VkFormat> colorAttachmentsFormats;
	pstd::vector<VkFormat> depthAttachmentFormat;
	pstd::vector<VkFormat> stencilAttachmentFormat;

	VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
};


#endif // !VULKAN_PIPELINE

