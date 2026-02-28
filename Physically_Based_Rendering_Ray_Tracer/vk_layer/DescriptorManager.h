#ifndef VULKAN_DESCRIPTOR_MANAGER
#define VULKAN_DESCRIPTOR_MANAGER

#include "base/base.h"
#include "util/pstd.h"

class DescriptorSetLayout {
	friend class DescriptorManager;
public:
	DescriptorSetLayout(){}
	DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
	DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

	void add_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count = 1, VkSampler* ImmutableSamplers = nullptr);

	void build();

	operator const VkDescriptorSetLayout& () const { return _layout; }

	~DescriptorSetLayout();

private:
	DescriptorSetLayout(VkDevice device);

	pstd::vector<VkDescriptorSetLayoutBinding> _bindings;

	VkDescriptorSetLayout _layout = VK_NULL_HANDLE;

	std::unordered_map<VkDescriptorType, uint32_t> type_num;

	VkDevice _device = VK_NULL_HANDLE;
};

class DescriptorSet {
	friend class DescriptorManager;
public:
	DescriptorSet(){}
	DescriptorSet(DescriptorSet&& other) noexcept;
	DescriptorSet& operator=(DescriptorSet&& other) noexcept;

	operator const VkDescriptorSet& () const { return _descriptorSet; }
	VkDescriptorSet get() const { return _descriptorSet; }


	void descriptor_write(uint32_t binding, VkDescriptorType type, const Image& image);

	void descriptor_write(uint32_t binding, VkDescriptorType type, const Buffer& buffer);

	void descriptor_write(uint32_t binding, const VkAccelerationStructureKHR& as);

	// Can only be called for once before every update_descriptor_set() invocation
	void descriptor_write(uint32_t binding, VkDescriptorType type, const pstd::vector<Texture>& textures);

private:
	DescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, DescriptorManager* manager);

	DescriptorManager* _manager = nullptr;

	VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};


class DescriptorManager {
	friend class DescriptorSet;

public:
	DescriptorManager(Context& context);

	void init();

	~DescriptorManager();

	DescriptorSetLayout& create_null_descriptor_set_layout(const std::string& name);

	// Must be called after layout bindings have been added and layouts have been built
	void init_descriptor_pool(uint32_t max_sets_num);

	DescriptorSet& allocate_descriptor_set(const std::string& layout_name);

	pstd::vector<VkDescriptorSetLayout> get_descriptor_set_layouts(const pstd::vector<std::string>& layout_names);

	void update_descriptor_set();
	
private:
	struct DescriptorTextureWrite {
		pstd::vector<VkDescriptorImageInfo> writesImages;
	};

	pstd::vector<VkWriteDescriptorSet> writes;

	std::list<VkDescriptorImageInfo> writesImage;
	std::list<VkDescriptorBufferInfo> writesBuffer;
	std::list<VkWriteDescriptorSetAccelerationStructureKHR> writesAS;
	std::list<DescriptorTextureWrite> writesTexture;

	

	pstd::vector<DescriptorSet> sets;

	std::unordered_map<std::string, DescriptorSetLayout> layouts;

	std::unordered_map<VkDescriptorType, uint32_t> total_type_num;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	
	Context& _context;
};

#endif // !VULKAN_DESCRIPTOR_MANAGER

