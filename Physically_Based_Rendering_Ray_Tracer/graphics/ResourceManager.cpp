#include "ResourceManager.h"

#include "vk_layer/Image.h"
#include "vk_layer/Buffer.h"
#include "vk_layer/Context.h"
#include "vk_layer/Texture.h"
#include "vk_layer/Pipeline.h"
#include "vk_layer/ASManager.h"
#include "vk_layer/CommandPool.h"
#include "vk_layer/PipelineManager.h"
#include "vk_layer/DescriptorManager.h"
#include "vk_layer/VkMemoryAllocator.h"

#include <fstream>

ResourceManager::ResourceManager(Context& context): _context(context){}
ResourceManager::~ResourceManager() = default;

void ResourceManager::register_resources(Buffer&& buffer, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, GLSLLayoutInfo layoutInfo) {
	if (flags & RF_PER_FRAME)
		throw std::runtime_error("ResourceManager::PER_FRAME resources can't be regsitered through this function");
	if (flags & RF_TEMPORAL)
		throw std::runtime_error("ResourceManager::TEMPORAL resources can't be registered through this function");

	// Resources movement & Register searching list
	buffers.push_back(std::move(buffer));
	resources_searching_list.insert({ binding_name, &buffers.back() });

	
	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Descriptor Set
		bool skipDescriptor = (pipeline->get_pipeline_type() == Pipeline::PipelineType::GRAPHICS) && (flags & RF_BIND_VERTEX);

		if (!skipDescriptor && (flags &RF_BIND_DESCRIPTOR)) {
			// Create descriptor layout & Add binding 
			ResourceFlag layout_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
			std::string layout_name = rf_flag_to_string(layout_flag);

			if (layouts.find(layout_flag) == layouts.end()) {
				uint32_t set_num = 1;
				layouts.insert({ layout_flag, &_context.descriptorManager().create_null_descriptor_set_layout(layout_name, set_num) });
				
				set_descriptor_lists.insert({ layout_flag, pstd::vector<ResourceDefination>()});
			}

			uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size());
			VkShaderStageFlags shader_stages = pipeline->supported_shader_stages();
			layouts[layout_flag]->add_binding(binding, descriptorType, shader_stages, 1);

			// Push in binding_list, for later header file generation
			set_descriptor_lists[layout_flag].push_back({ binding, descriptorType, VK_FORMAT_UNDEFINED, layoutInfo});


			// Record resource handle
			DescriptorHandle descriptor;
			descriptor.flags = layout_flag;
			descriptor.binding = binding;
			descriptor.set_idx = 0;
			descriptor.type = descriptorType;
			descriptor.ptr = &buffers.back();

			descriptors.push_back(descriptor);

			// Check if resource is "window size related"
			if (flags & RF_WINDOW_SIZE_RELATED)
				window_size_related_resources_caches.push_back(descriptor);
		}

		if (flags & RF_BIND_VERTEX) {
			VkVertexInputBindingDescription binding{};
			binding.binding = static_cast<uint32_t>(vertexInputBindings.size());
			binding.stride = buffers.back().stride;
			binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription attribute{};
			attribute.location = static_cast<uint32_t>(vertexInputAttributes.size());
			attribute.binding = binding.binding;
			attribute.format = buffers.back().format;
			attribute.offset = 0;

			vertexInputBindings.push_back(binding);
			vertexInputAttributes.push_back(attribute);

			vertex_input_list.push_back(binding_name);
		}
		
	}
	
}

void ResourceManager::register_resources(Image&& image, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkImageLayout targetLayout, GLSLLayoutInfo layoutInfo) {
	if (flags & RF_PER_FRAME)
		throw std::runtime_error("ResourceManager::PER_FRAME resources can't be regsitered through this function");
	if (flags & RF_TEMPORAL)
		throw std::runtime_error("ResourceManager::TEMPORAL resources can't be registered through this function");

	// Image Layout Transition
	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);
	image.transition_layout(cmdBuffer, targetLayout);
	cmdBuffer.end_and_submit(_context.gc_queue(), true);

	// Resources movement & Register searching list
	images.push_back(std::move(image));
	resources_searching_list.insert({ binding_name, &images.back() });

	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Create descriptor layout & Add binding 
		ResourceFlag layout_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string layout_name = rf_flag_to_string(layout_flag);

		if (layouts.find(layout_flag) == layouts.end()) {
			uint32_t set_num = 1;
			layouts.insert({ layout_flag, &_context.descriptorManager().create_null_descriptor_set_layout(layout_name, set_num) });

			set_descriptor_lists.insert({ layout_flag, pstd::vector<ResourceDefination>() });
		}


		uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size());
		VkShaderStageFlags shader_stages = pipeline->supported_shader_stages();
		layouts[layout_flag]->add_binding(binding, descriptorType, shader_stages, 1);

		// Push in binding_list, for later header file generation
		set_descriptor_lists[layout_flag].push_back({ binding, descriptorType, images.back().format, layoutInfo});


		// Record resource handle
		DescriptorHandle descriptor;
		descriptor.flags = layout_flag;
		descriptor.binding = binding;
		descriptor.set_idx = 0;
		descriptor.type = descriptorType;
		descriptor.ptr = &images.back();

		descriptors.push_back(descriptor);

		// Check if resource is "window size related"
		if (flags & RF_WINDOW_SIZE_RELATED)
			window_size_related_resources_caches.push_back(descriptor);
	}
}

void ResourceManager::register_resources(TLAS&& tlas, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, GLSLLayoutInfo layoutInfo) {
	// Resources movement & Register searching list
	tlas_list.push_back(std::move(tlas));
	resources_searching_list.insert({ binding_name, &tlas_list.back() });


	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Create descriptor layout & Add binding 
		ResourceFlag layout_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string layout_name = rf_flag_to_string(layout_flag);

		if (layouts.find(layout_flag) == layouts.end()) {
			uint32_t set_num = 1;
			layouts.insert({ layout_flag, &_context.descriptorManager().create_null_descriptor_set_layout(layout_name, set_num) });

			set_descriptor_lists.insert({ layout_flag, pstd::vector<ResourceDefination>() });
		}

		uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size());
		VkShaderStageFlags shader_stages = pipeline->supported_shader_stages();
		layouts[layout_flag]->add_binding(binding, descriptorType, shader_stages, 1);

		// Push in binding_list£¬for later header file generation
		set_descriptor_lists[layout_flag].push_back({ binding, descriptorType, VK_FORMAT_UNDEFINED, layoutInfo });


		// Record resource handle
		DescriptorHandle descriptor;
		descriptor.flags = layout_flag;
		descriptor.binding = binding;
		descriptor.set_idx = 0;
		descriptor.type = descriptorType;
		descriptor.ptr = &tlas_list.back();

		descriptors.push_back(descriptor);
	}
}

void ResourceManager::register_resources(pstd::vector<Texture>&& textures, pstd::vector<Sampler>&& samplers, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, GLSLLayoutInfo layoutInfo) {
	// Resources movement & Register searching list
	textures_lists.push_back(std::move(textures));
	samplers_lists.push_back(std::move(samplers));
	resources_searching_list.insert({ binding_name, &textures_lists.back() });


	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Create descriptor layout & Add binding 
		ResourceFlag layout_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string layout_name = rf_flag_to_string(layout_flag);

		if (layouts.find(layout_flag) == layouts.end()) {
			uint32_t set_num = 1;
			layouts.insert({ layout_flag, &_context.descriptorManager().create_null_descriptor_set_layout(layout_name, set_num) });

			set_descriptor_lists.insert({ layout_flag, pstd::vector<ResourceDefination>() });
		}

		uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size());
		VkShaderStageFlags shader_stages = pipeline->supported_shader_stages();
		layouts[layout_flag]->add_binding(binding, descriptorType, shader_stages, textures_lists.back().size());

		// Push in binding_list£¬for later header file generation
		set_descriptor_lists[layout_flag].push_back({ binding, descriptorType, VK_FORMAT_UNDEFINED, layoutInfo });


		// Record resource handle
		DescriptorHandle descriptor;
		descriptor.flags = layout_flag;
		descriptor.binding = binding;
		descriptor.set_idx = 0;
		descriptor.type = descriptorType;
		descriptor.ptr = &textures_lists.back();

		descriptors.push_back(descriptor);
	}
}

void ResourceManager::register_resources(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkImageLayout targetLayout, GLSLLayoutInfo layoutInfo) {

	uint32_t resourceCount;
	if (flags & RF_PER_FRAME) resourceCount = MAX_FRAMES_IN_FLIGHT;
	else if (flags & RF_TEMPORAL) resourceCount = 2;
	else resourceCount = 1;

	pstd::vector<Image*> imagePtrs;
	for (uint32_t i = 0; i < resourceCount; ++i) {
		images.push_back(_context.memAllocator().create_image(extent, format, tiling, usage, properties));

		Image* ptr = &images.back();
		imagePtrs.push_back(ptr);

		// Register searching list
		std::string resource_name = binding_name;
		if (resourceCount > 1) resource_name += std::to_string(i);
		resources_searching_list.insert({ resource_name, ptr });
	}

	// Image Layout Transition
	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);
	for (auto& imagePtr : imagePtrs) {
		imagePtr->transition_layout(cmdBuffer, targetLayout);
	}
	cmdBuffer.end_and_submit(_context.gc_queue(), true);

	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		ResourceFlag layout_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string layout_name = rf_flag_to_string(layout_flag);
		uint32_t set_num;
		if (layout_flag & RF_PER_FRAME) set_num = MAX_FRAMES_IN_FLIGHT;
		else if (layout_flag & RF_TEMPORAL) set_num = 2;
		else set_num = 1;

		if (layouts.find(layout_flag) == layouts.end()) {
			layouts.insert({ layout_flag, &_context.descriptorManager().create_null_descriptor_set_layout(layout_name, set_num) });

			set_descriptor_lists.insert({ layout_flag , pstd::vector<ResourceDefination>() });
		}

		pstd::vector<uint32_t> bindings;

		for (uint32_t i = 0; i < resourceCount; ++i) {
			if (layout_flag & RF_PER_FRAME || layout_flag & RF_STATIC) {
				uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size());
				bindings.push_back(binding);
			}
			else if(layout_flag & RF_TEMPORAL) {
				uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size()) + i;
				bindings.push_back(binding);
			}
		}

		
		VkShaderStageFlags shader_stages = pipeline->supported_shader_stages();
		if (layout_flag & RF_PER_FRAME || layout_flag & RF_STATIC) {
			layouts[layout_flag]->add_binding(bindings[0], descriptorType, shader_stages, 1);
			set_descriptor_lists[layout_flag].push_back({ bindings[0], descriptorType, format, layoutInfo});
		}
		else if (layout_flag & RF_TEMPORAL) {
			layouts[layout_flag]->add_binding(bindings[0], descriptorType, shader_stages, 1);
			GLSLLayoutInfo info0 = layoutInfo;
			info0.blockName += "_CURRENT_FRAME";
			info0.memberName += "_current";
			set_descriptor_lists[layout_flag].push_back({ bindings[0], descriptorType, format, info0 });

			layouts[layout_flag]->add_binding(bindings[1], descriptorType, shader_stages, 1);
			GLSLLayoutInfo info1 = layoutInfo;
			info1.blockName += "_LAST_FRAME";
			info1.memberName += "_prev";
			set_descriptor_lists[layout_flag].push_back({ bindings[1], descriptorType, format, info1 });
		}

		if (layout_flag & RF_PER_FRAME || layout_flag & RF_STATIC) {
			for (uint32_t i = 0; i < resourceCount; ++i) {
				DescriptorHandle descriptor;
				descriptor.flags = layout_flag;
				descriptor.binding = bindings[i];
				descriptor.set_idx = i;
				descriptor.type = descriptorType;
				descriptor.ptr = imagePtrs[i];

				descriptors.push_back(descriptor);

				// Check if resource is "window size related"
				if (flags & RF_WINDOW_SIZE_RELATED)
					window_size_related_resources_caches.push_back(descriptor);
			}
		}
		else if (layout_flag & RF_TEMPORAL) {
			for (uint32_t i = 0; i < resourceCount; ++i) {
				// Set 0
				DescriptorHandle descriptor;
				descriptor.flags = layout_flag;
				descriptor.binding = bindings[i];
				descriptor.set_idx = 0;
				descriptor.type = descriptorType;
				descriptor.ptr = imagePtrs[i];

				descriptors.push_back(descriptor);

				// Check if resource is "window size related"
				if (flags & RF_WINDOW_SIZE_RELATED)
					window_size_related_resources_caches.push_back(descriptor);

				// Set 1
				descriptor.flags = layout_flag;
				descriptor.binding = bindings[resourceCount - 1 - i];
				descriptor.set_idx = 1;
				descriptor.type = descriptorType;
				descriptor.ptr = imagePtrs[i];

				descriptors.push_back(descriptor);

				// Check if resource is "window size related"
				if (flags & RF_WINDOW_SIZE_RELATED)
					window_size_related_resources_caches.push_back(descriptor);
			}
		}
	}

}

// For creating Buffer inside the function
void ResourceManager::register_resources(VkDeviceSize size, VkDeviceSize stride, VkFormat format, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, GLSLLayoutInfo layoutInfo) {

	uint32_t resourceCount;
	if (flags & RF_PER_FRAME) resourceCount = MAX_FRAMES_IN_FLIGHT;
	else if (flags & RF_TEMPORAL) resourceCount = 2;	
	else resourceCount = 1;

	pstd::vector<Buffer*> bufferPtrs;
	for (uint32_t i = 0; i < resourceCount; ++i) {
		buffers.push_back(_context.memAllocator().create_buffer(size, stride, format, usage, properties));

		Buffer* ptr = &buffers.back();
		bufferPtrs.push_back(ptr);

		// Register searching list
		std::string resource_name = binding_name;
		if (resourceCount > 1) resource_name += std::to_string(i);
		resources_searching_list.insert({ resource_name, ptr });
	}

	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		if (flags & RF_BIND_DESCRIPTOR) {
			ResourceFlag layout_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
			std::string set_name = rf_flag_to_string(layout_flag);
			uint32_t set_num;
			if (layout_flag & RF_PER_FRAME) set_num = MAX_FRAMES_IN_FLIGHT;
			else if (layout_flag & RF_TEMPORAL) set_num = 2;
			else set_num = 1;



			if (layouts.find(layout_flag) == layouts.end()) {
				layouts.insert({ layout_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

				set_descriptor_lists.insert({ layout_flag, pstd::vector<ResourceDefination>() });
			}

			pstd::vector<uint32_t> bindings;
			for (uint32_t i = 0; i < resourceCount; ++i) {
				if (layout_flag & RF_PER_FRAME || layout_flag & RF_STATIC) {
					uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size());
					bindings.push_back(binding);
				}
				else if (layout_flag & RF_TEMPORAL) {
					uint32_t binding = static_cast<uint32_t>(set_descriptor_lists[layout_flag].size()) + i;
					bindings.push_back(binding);
				}
			}

			VkShaderStageFlags shader_stages = pipeline->supported_shader_stages();
			if (layout_flag & RF_PER_FRAME || layout_flag & RF_STATIC) {
				layouts[layout_flag]->add_binding(bindings[0], descriptorType, shader_stages, 1);
				set_descriptor_lists[layout_flag].push_back({ bindings[0], descriptorType, VK_FORMAT_UNDEFINED, layoutInfo });
			}
			else if (layout_flag & RF_TEMPORAL) {
				// Temporal Resources' names should not be null
				layouts[layout_flag]->add_binding(bindings[0], descriptorType, shader_stages, 1);
				GLSLLayoutInfo info0 = layoutInfo;
				info0.blockName += "_CURRENT_FRAME";
				info0.memberName += "_current";
				set_descriptor_lists[layout_flag].push_back({ bindings[0], descriptorType, VK_FORMAT_UNDEFINED, info0 });

				layouts[layout_flag]->add_binding(bindings[1], descriptorType, shader_stages, 1);
				GLSLLayoutInfo info1 = layoutInfo;
				info1.blockName += "_LAST_FRAME";
				info1.memberName += "_prev";
				set_descriptor_lists[layout_flag].push_back({ bindings[1], descriptorType, VK_FORMAT_UNDEFINED, info1 });
			}

			if (layout_flag & RF_PER_FRAME || layout_flag & RF_STATIC) {
				for (uint32_t i = 0; i < resourceCount; ++i) {
					DescriptorHandle descriptor;
					descriptor.flags = layout_flag;
					descriptor.binding = bindings[i];
					descriptor.set_idx = i;
					descriptor.type = descriptorType;
					descriptor.ptr = bufferPtrs[i];

					descriptors.push_back(descriptor);

					// Check if resource is "window size related"
					if (flags & RF_WINDOW_SIZE_RELATED)
						window_size_related_resources_caches.push_back(descriptor);
				}
			}
			else if (layout_flag & RF_TEMPORAL) {
				for (uint32_t i = 0; i < resourceCount; ++i) {
					// Set 0
					DescriptorHandle descriptor;
					descriptor.flags = layout_flag;
					descriptor.binding = bindings[i];
					descriptor.set_idx = 0;
					descriptor.type = descriptorType;
					descriptor.ptr = bufferPtrs[i];

					descriptors.push_back(descriptor);

					// Check if resource is "window size related"
					if (flags & RF_WINDOW_SIZE_RELATED)
						window_size_related_resources_caches.push_back(descriptor);

					// Set 1
					descriptor.flags = layout_flag;
					descriptor.binding = bindings[resourceCount - 1 - i];
					descriptor.set_idx = 1;
					descriptor.type = descriptorType;
					descriptor.ptr = bufferPtrs[i];

					descriptors.push_back(descriptor);

					// Check if resource is "window size related"
					if (flags & RF_WINDOW_SIZE_RELATED)
						window_size_related_resources_caches.push_back(descriptor);
				}
			}
		}

		if (flags & RF_BIND_VERTEX) {
			VkVertexInputBindingDescription binding{};
			binding.binding = static_cast<uint32_t>(vertexInputBindings.size());
			binding.stride = buffers.back().stride;
			binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription attribute{};
			attribute.location = static_cast<uint32_t>(vertexInputAttributes.size());
			attribute.binding = binding.binding;
			attribute.format = buffers.back().format;
			attribute.offset = 0;

			vertexInputBindings.push_back(binding);
			vertexInputAttributes.push_back(attribute);

			vertex_input_list.push_back(binding_name);
		}
	}
}


void ResourceManager::build() {
	// Init Descriptor Pool
	_context.descriptorManager().init_descriptor_pool();

	// Build Layout & Allocate Descriptor Set
	for (const auto& flag_layout : layouts) {
		// Build Layout
		flag_layout.second->build();

		ResourceFlag flag = flag_layout.first;
		uint32_t set_num;
		if (flag & RF_PER_FRAME) set_num = MAX_FRAMES_IN_FLIGHT;
		else if (flag & RF_TEMPORAL) set_num = 2;
		else set_num = 1;

		for (uint32_t i = 0; i < set_num; ++i) {
			// Allocate Descriptor Set
			std::string layout_name = rf_flag_to_string(flag);
			DescriptorSet& set = _context.descriptorManager().allocate_descriptor_set(layout_name);

			std::string set_name = layout_name;
			if (set_num > 1) set_name += std::to_string(i);

			sets.insert({ set_name, &set });
		}
	}

	// Add Descriptor Layout to pipeline

	pstd::vector<ResourceFlag> fr_flags{ RF_PER_FRAME, RF_STATIC, RF_TEMPORAL };

	for (const auto& pipeline_id : pipeline_ids) {
		auto& pipeline = _context.pipelineManager().get(pipeline_id);
		pstd::vector<VkDescriptorSetLayout> tmp_layouts;

		for (const auto& fr_flag : fr_flags) {
			ResourceFlag layout_key = fr_flag | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR;
			if (auto It = layouts.find(layout_key); It != layouts.end())
				tmp_layouts.push_back(*It->second);
		}

		pipeline.register_descriptor_set_layout(tmp_layouts);
	}

	// Write In Descriptor 
	for (const auto& descriptor : descriptors) {
		ResourceFlag flag = descriptor.flags;
		bool if_multi = ((flag & RF_PER_FRAME) && (MAX_FRAMES_IN_FLIGHT > 1)) || (flag & RF_TEMPORAL);
		
		std::string set_name = rf_flag_to_string(flag);
		if (if_multi) set_name += std::to_string(descriptor.set_idx);

		DescriptorSet* set = sets[set_name];

		std::visit([&](auto* ptr) {
			using T = std::decay_t<decltype(*ptr)>;
			if constexpr (std::is_same_v<T, Buffer>) {
				set->descriptor_write(descriptor.binding, descriptor.type, *ptr);
			}
			else if constexpr (std::is_same_v<T, Image>) {
				set->descriptor_write(descriptor.binding, descriptor.type, *ptr);
			}
			else if constexpr (std::is_same_v<T, TLAS>) {
				set->descriptor_write(descriptor.binding, *ptr);
			}
			else if constexpr (std::is_same_v<T, pstd::vector<Texture>>) {
				set->descriptor_write(descriptor.binding, descriptor.type, *ptr);
			}
			}, descriptor.ptr);
	}
	
	_context.descriptorManager().update_descriptor_set();

	// Generate Header file by pipeline
	for (const auto& pipeline_id : pipeline_ids) {
		generate_header(pipeline_id);
	}

	set_descriptor_lists.clear();
}


void ResourceManager::rebuild_window_size_related_resources(VkExtent2D extent) {
	std::unordered_set<Image*> rebuilt_image;
	std::unordered_set<Buffer*> rebuilt_buffer;

	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin();

	for (auto& handle : window_size_related_resources_caches) {
		// Rebuild Resources
		std::visit([&](auto* ptr) {
			using T = std::decay_t<decltype(*ptr)>;
			if constexpr (std::is_same_v<T, Image>) {
				if (rebuilt_image.find(ptr) == rebuilt_image.end()) {
					*ptr = _context.memAllocator().create_image(extent, ptr->format, ptr->tiling, ptr->usage, ptr->memoryProperties);

					ptr->transition_layout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

					rebuilt_image.insert(ptr);
				}				
			}
			else if constexpr (std::is_same_v<T, Buffer>) {
				if (rebuilt_buffer.find(ptr) == rebuilt_buffer.end()) {
					*ptr = _context.memAllocator().create_buffer(
						static_cast<VkDeviceSize>(extent.height * extent.width) * ptr->stride,
						ptr->stride,
						ptr->format,
						ptr->usage,
						ptr->memoryProperties
					);

					rebuilt_buffer.insert(ptr);
				}
			}
		}, handle.ptr);

		// Write in descriptor set
		std::string set_name = rf_flag_to_string(handle.flags);
		bool if_multi = ((handle.flags & RF_PER_FRAME) && (MAX_FRAMES_IN_FLIGHT > 1)) || (handle.flags & RF_TEMPORAL);
		if (if_multi) set_name += std::to_string(handle.set_idx);

		DescriptorSet& set = *sets[set_name];
		std::visit([&](auto* ptr) {
			using T = std::decay_t<decltype(*ptr)>;
			if constexpr (std::is_same_v<T, Image>) {
				set.descriptor_write(handle.binding, handle.type, *ptr);
			}
			else if constexpr (std::is_same_v<T, Buffer>) {
				set.descriptor_write(handle.binding, handle.type, *ptr);
			}
			}, handle.ptr);

	}

	cmdBuffer.end_and_submit(_context.gc_queue(), true);

	_context.descriptorManager().update_descriptor_set();
}

pstd::vector<VkDescriptorSet> ResourceManager::get_descriptor_sets(uint32_t pipeline_id, uint32_t frame_idx, uint32_t temporal_idx) {


	std::string static_set_name = rf_flag_to_string(RF_STATIC | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR) ;
	std::string dynamic_set_name = rf_flag_to_string(RF_PER_FRAME | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR);
	if (MAX_FRAMES_IN_FLIGHT > 1) dynamic_set_name += std::to_string(frame_idx);
	std::string temporal_set_name = rf_flag_to_string(RF_TEMPORAL | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR) + std::to_string(temporal_idx);


	pstd::vector<VkDescriptorSet> pipeline_sets;
	if (auto It = sets.find(dynamic_set_name); It != sets.end())
		pipeline_sets.push_back(*It->second);
	if (auto It = sets.find(static_set_name); It != sets.end())
		pipeline_sets.push_back(*It->second);
	if (auto It = sets.find(temporal_set_name); It != sets.end())
		pipeline_sets.push_back(*It->second);
	return pipeline_sets;
}

pstd::vector<VkDescriptorSet> ResourceManager::get_descriptor_sets(std::string pipeline_name, uint32_t frame_idx, uint32_t temporal_idx)
{
	return get_descriptor_sets(_context.pipelineManager().get_id(pipeline_name), frame_idx, temporal_idx);
}

const pstd::vector<VkVertexInputBindingDescription>& ResourceManager::get_vertex_input_binding() {
	return vertexInputBindings;
}

const pstd::vector< VkVertexInputAttributeDescription>& ResourceManager::get_vertex_input_attribute() {
	return vertexInputAttributes;
}


std::string ResourceManager::rf_flag_to_string(ResourceFlag flags) {
	std::string result = "PIPELINE_";
	
	
	uint32_t pipeline_id = static_cast<uint32_t>(flags >> 32);
	result += _context.pipelineManager().get(pipeline_id).get_name();

	if (flags & RF_BIND_VERTEX) result += "_VERTEX";
	else result += "_DESCRIPTOR_SET";


	if (flags & RF_PER_FRAME) result += "_DYNAMIC";
	else if (flags & RF_STATIC) result += "_STATIC";
	else if (flags & RF_TEMPORAL) result += "_TEMPORAL";

	return result;
}

ResourceFlag ResourceManager::rf_get_frequency(ResourceFlag flags) { return flags & RF_FREQUENCY_MASK; }

ResourceFlag ResourceManager::rf_get_bind_point(ResourceFlag flags) { return flags & RF_BINDPOINT_MASK; }

ResourceFlag ResourceManager::rf_get_pipeline(uint32_t pipeline_id) {
	ResourceFlag flag = static_cast<ResourceFlag>(pipeline_id);
	flag <<= 32;

	return flag;
}

std::string ResourceManager::resolve_layout_info(uint32_t binding, uint32_t set, VkDescriptorType type, VkFormat format, const GLSLLayoutInfo& layout_info) {
	std::string layout = "layout(binding = " + std::to_string(binding) + ", set = " + std::to_string(set);

	switch (type) {
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
		layout += ", scalar)";
		std::string block = layout_info.blockName.empty() ? (layout_info.memberType + "_Block") : layout_info.blockName;
		std::string arr = layout_info.isArray ? "[]" : "";
		return layout + " buffer " + block + " {\n\t" + layout_info.memberType + " " + layout_info.memberName + arr +";\n};\n";
	}
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
		layout += ")";
		std::string block = layout_info.blockName.empty() ? (layout_info.memberType + "_Block") : layout_info.blockName;
		return layout + " uniform " + block + " {\n\t" + layout_info.memberType + " " + layout_info.memberName + ";\n};\n";
	}
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
		layout += ")";
		std::string arr = layout_info.isArray ? "[]" : "";
		return layout + " uniform sampler2D " + layout_info.memberName + arr + ";\n";
	}
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
		layout += (", " + vk_format_to_glsl_qualifier(format) + ")");
		return layout + " uniform image2D " + layout_info.memberName + ";\n";
	}
	case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
		layout += ")";
		return layout + " uniform accelerationStructureEXT " + layout_info.memberName + ";\n";
	}
	default:
		throw std::runtime_error("ResourceManager::Resource Type is not supported");
	}
}

std::string ResourceManager::vk_format_to_glsl_qualifier(VkFormat format) {
	switch (format) {
	case VK_FORMAT_R32G32B32A32_SFLOAT: return "rgba32f";
	case VK_FORMAT_R16G16B16A16_SFLOAT: return "rgba16f";
	case VK_FORMAT_R8G8B8A8_UNORM: return "rgba8";
	case VK_FORMAT_R32_SFLOAT: return "r32f";
	case VK_FORMAT_R32_UINT: return "r32ui";
	default:
		throw std::runtime_error("ResourceManager::Image Format is not supported");
	}
}

void ResourceManager::generate_header(uint32_t pipeline_id) {
	std::string pipeline_name = _context.pipelineManager().get_name(pipeline_id);
	std::string filename = pipeline_name + "_header.glslh";
	std::ofstream out("./shader/generated/" + filename);
	if(!out.is_open())
		throw std::runtime_error("ResourceManager::Failed to open header file: " + filename);

	std::string guard = pipeline_name + "_HEADER_GLSLH";
	for (auto& c : guard) c = static_cast<char>(std::toupper(c));

	out << "// Auto-generated by ResourceManager for pipeline: " << pipeline_name << "\n";
	out << "#ifndef " << guard << "\n";
	out << "#define " << guard << "\n\n";

	// Required extensions
	out << "#extension GL_EXT_scalar_block_layout : require\n\n";

	// Include
	out << "#include \"../common/shaderCommon.glslh\"\n";
	out << "#include \"../../base/shader_base.h\"\n";

	// Descriptor Set
	pstd::vector<ResourceFlag> fr_flags{ RF_PER_FRAME, RF_STATIC, RF_TEMPORAL };
	uint32_t set_count = 0;
	for (const auto& fr_flag : fr_flags) {
		ResourceFlag layout_flag = fr_flag | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR;

		auto it = set_descriptor_lists.find(layout_flag);
		if (it == set_descriptor_lists.end())continue;

		const auto& defs = it->second;
		std::string set_label = rf_flag_to_string(layout_flag);

		out << "// " << set_label << " (set = " << set_count << ")\n";

		for (const auto& def : defs) {
			out << resolve_layout_info(def.binding, set_count, def.type, def.format, def.layout_info);
		}
		
		out << "\n";
		set_count++;
	}
	

	

	out << "#endif // " << guard << "\n";
	out.close();
}