#include "DescriptorManager.h"

#include "Context.h"
#include "Texture.h"
#include "Image.h"
#include "Buffer.h"

DescriptorSetLayout::DescriptorSetLayout(VkDevice device) :_device(device){}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept :_bindings(std::move(other._bindings)), type_num(std::move(other.type_num)), _layout(other._layout), _device(other._device) {
	other._bindings.clear();
	other.type_num.clear();
	other._layout = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout && other) noexcept {
	if (&other != this) {
		if (_layout != VK_NULL_HANDLE)vkDestroyDescriptorSetLayout(_device, _layout, nullptr);
	}

	_bindings = std::move(other._bindings);
	type_num = std::move(other.type_num);
	_layout = other._layout;
	_device = other._device;


	other._bindings.clear();
	other.type_num.clear();
	other._layout = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;

	return *this;
}

DescriptorSetLayout::~DescriptorSetLayout() {
	if (_layout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(_device, _layout, nullptr);
}

void DescriptorSetLayout::add_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count, VkSampler* ImmutableSamplers) {
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = type;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stage;
	layoutBinding.pImmutableSamplers = ImmutableSamplers;

	type_num[type] += count;

	_bindings.push_back(layoutBinding);
}


void DescriptorSetLayout::build() {
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = _bindings.size();
	layoutInfo.pBindings = _bindings.data();

	if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_layout) != VK_SUCCESS)
		throw std::runtime_error("DescriptorSetLayout::Failed to create descriptor set layout");
	
	_bindings.clear();
}

DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorSet descriptorSet):_device(device), _descriptorSet(descriptorSet){}

DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept: _descriptorSet(other._descriptorSet), _device(other._device){
	other._descriptorSet = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {
	_descriptorSet = other._descriptorSet;
	_device = other._device;

	other._descriptorSet = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;

	return *this;
}




DescriptorManager::DescriptorManager(Context& context):_context(context){}

void DescriptorManager::init(){}

DescriptorManager::~DescriptorManager() {
	vkDestroyDescriptorPool(_context, descriptorPool, nullptr);

	LOG_STREAM("DescriptorManager") << "DescriptorManager has been deconstructed" << std::endl;
}

DescriptorSetLayout& DescriptorManager::create_null_descriptor_set_layout(const std::string& name) {
	layouts.insert({ name, DescriptorSetLayout(_context) });
	
	return layouts.find(name)->second;
}

void DescriptorManager::init_descriptor_pool(uint32_t max_sets_num) {
	for (const auto& layout : layouts) {
		for (const auto& ele : layout.second.type_num) {
			total_type_num[ele.first] += ele.second;
		}
	}

	pstd::vector<VkDescriptorPoolSize> poolSizes;
	for (const auto& ele : total_type_num) {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = ele.first;
		poolSize.descriptorCount = ele.second;

		poolSizes.push_back(poolSize);
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(((max_sets_num == 0) ? layouts.size() : max_sets_num));
	poolInfo.flags = 0;
	poolInfo.pNext = nullptr;
	
	if (vkCreateDescriptorPool(_context, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("DescriptorManager::Failed to create Descriptor pool");
}

DescriptorSet& DescriptorManager::allocate_descriptor_set(const std::string& layout_name, const std::string& set_name) {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layouts.find(layout_name)->second._layout;
	allocInfo.pNext = nullptr;

	VkDescriptorSet set;
	if (vkAllocateDescriptorSets(_context, &allocInfo, &set) != VK_SUCCESS)
		throw std::runtime_error("DescriptorManager::Failed to allocate descriptor set");

	sets.insert({ set_name, DescriptorSet(_context, set) });
	return sets.find(set_name)->second;
}

pstd::vector<VkDescriptorSetLayout> DescriptorManager::get_descriptor_set_layouts(const pstd::vector<std::string>& layout_names) {
	pstd::vector<VkDescriptorSetLayout> vkLayouts(layout_names.size());
	
	for (uint32_t i = 0; i < layout_names.size(); ++i) {
		vkLayouts[i] = layouts.find(layout_names[i])->second;
	}
	return vkLayouts;
}

void DescriptorManager::descriptor_write(const std::string& set_name, uint32_t binding, VkDescriptorType type, const Image& image) {

	writesImage.push_back(VkDescriptorImageInfo{});
	writesImage.back().imageLayout = image.layout;
	writesImage.back().imageView = image;

	writes.push_back(VkWriteDescriptorSet{});
	writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes.back().dstSet = sets.find(set_name)->second;
	writes.back().dstBinding = binding;
	writes.back().dstArrayElement = 0;
	writes.back().descriptorType = type;
	writes.back().descriptorCount = 1;
	writes.back().pImageInfo = &writesImage.back();
	
}

void DescriptorManager::descriptor_write(const std::string& set_name, uint32_t binding, VkDescriptorType type, const Buffer& buffer) {
	writesBuffer.push_back(VkDescriptorBufferInfo{});
	writesBuffer.back().buffer = buffer;
	writesBuffer.back().offset = 0;
	writesBuffer.back().range = VK_WHOLE_SIZE;

	writes.push_back(VkWriteDescriptorSet{});
	writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes.back().dstSet = sets.find(set_name)->second;
	writes.back().dstBinding = binding;
	writes.back().dstArrayElement = 0;
	writes.back().descriptorType = type;
	writes.back().descriptorCount = 1;
	writes.back().pBufferInfo = &writesBuffer.back();

}

void DescriptorManager::descriptor_write(const std::string& set_name, uint32_t binding, const VkAccelerationStructureKHR& as) {
	writesAS.push_back(VkWriteDescriptorSetAccelerationStructureKHR{});
	writesAS.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	writesAS.back().accelerationStructureCount = 1;
	writesAS.back().pAccelerationStructures = &as;


	writes.push_back(VkWriteDescriptorSet{});
	writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes.back().dstSet = sets.find(set_name)->second;
	writes.back().dstBinding = binding;
	writes.back().dstArrayElement = 0;
	writes.back().descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	writes.back().descriptorCount = 1;
	writes.back().pNext = &writesAS.back();
}

void DescriptorManager::descriptor_write(const std::string& set_name, uint32_t binding, VkDescriptorType type, const pstd::vector<Texture>& textures) {
	writesTexture.clear();
	writesTexture.reserve(textures.size());
	for (const auto& texture : textures) {
		writesTexture.push_back(VkDescriptorImageInfo{});
		writesTexture.back().imageLayout = texture.image().layout;
		writesTexture.back().imageView = texture.image();
		writesTexture.back().sampler = texture.sampler();
	}

	writes.push_back(VkWriteDescriptorSet{});
	writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes.back().dstSet = sets.find(set_name)->second;
	writes.back().dstBinding = binding;
	writes.back().dstArrayElement = 0;
	writes.back().descriptorType = type;
	writes.back().descriptorCount = static_cast<uint32_t>(writesTexture.size());
	writes.back().pImageInfo = writesTexture.data();
	
}



void DescriptorManager::update_descriptor_set(){
	vkUpdateDescriptorSets(
		_context,
		static_cast<uint32_t>(writes.size()),
		writes.data(),
		0, nullptr
	);

	writes.clear();
	writesBuffer.clear();
	writesImage.clear();
	writesAS.clear();
	writesTexture.clear();
}