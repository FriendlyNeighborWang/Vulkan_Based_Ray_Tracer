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

void ResourceManager::register_resources(Buffer&& buffer, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages) {
	if (flags & RF_PER_FRAME)
		throw std::runtime_error("ResourceManager::PER_FRAME buffers can't be regsitered through this function");

	// Resources movement & Register searching list
	buffers.push_back(std::move(buffer));
	resources_searching_list.insert({ binding_name, &buffers.back() });

	
	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Descriptor Set
		if (flags & RF_BIND_DESCRIPTOR) {
			// Create descriptor layout & Add binding 
			ResourceFlag set_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
			std::string set_name = rf_flag_to_string(set_flag);
			uint32_t set_num = (set_flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

			if (layouts.find(set_flag) == layouts.end()) {
				layouts.insert({ set_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

				layout_binding_lists.insert({ set_flag, pstd::vector<std::string>() });
			}

			uint32_t binding = static_cast<uint32_t>(layout_binding_lists[set_flag].size());
			VkShaderStageFlags real_stages = stages & pipeline->supported_shader_stages();
			layouts[set_flag]->add_binding(binding, descriptorType, real_stages, 1);

			// Push in binding_list, for later header file generation
			layout_binding_lists[set_flag].push_back(set_name + "_" + binding_name);


			// Record resource handle
			DescriptorHandle descriptor;
			descriptor.flags = set_flag;
			descriptor.binding = binding;
			descriptor.idx = 0;
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

void ResourceManager::register_resources(Image&& image, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages, VkImageLayout targetLayout) {
	if (flags & RF_PER_FRAME)
		throw std::runtime_error("ResourceManager::PER_FRAME images can't be regsitered through this function");

	// Image Layout Transition
	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);
	image.transition_layout(_context, cmdBuffer, targetLayout);
	cmdBuffer.end_and_submit(_context.gc_queue(), true);

	// Resources movement & Register searching list
	images.push_back(std::move(image));
	resources_searching_list.insert({ binding_name, &images.back() });

	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Create descriptor layout & Add binding 
		ResourceFlag set_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string set_name = rf_flag_to_string(set_flag);
		uint32_t set_num = (set_flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

		if (layouts.find(set_flag) == layouts.end()) {
			layouts.insert({ set_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

			layout_binding_lists.insert({ set_flag, pstd::vector<std::string>() });
		}

		uint32_t binding = static_cast<uint32_t>(layout_binding_lists[set_flag].size());
		VkShaderStageFlags real_stages = stages & pipeline->supported_shader_stages();
		layouts[set_flag]->add_binding(binding, descriptorType, real_stages, 1);

		// Push in binding_list£¬for later header file generation
		layout_binding_lists[set_flag].push_back(set_name + "_" + binding_name);


		// Record resource handle
		DescriptorHandle descriptor;
		descriptor.flags = set_flag;
		descriptor.binding = binding;
		descriptor.idx = 0;
		descriptor.type = descriptorType;
		descriptor.ptr = &images.back();

		descriptors.push_back(descriptor);

		// Check if resource is "window size related"
		if (flags & RF_WINDOW_SIZE_RELATED)
			window_size_related_resources_caches.push_back(descriptor);
	}
}

void ResourceManager::register_resources(TLAS&& tlas, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages) {
	// Resources movement & Register searching list
	tlas_list.push_back(std::move(tlas));
	resources_searching_list.insert({ binding_name, &tlas_list.back() });


	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Create descriptor layout & Add binding 
		ResourceFlag set_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string set_name = rf_flag_to_string(set_flag);
		uint32_t set_num = (set_flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

		if (layouts.find(set_flag) == layouts.end()) {
			layouts.insert({ set_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

			layout_binding_lists.insert({ set_flag, pstd::vector<std::string>() });
		}

		uint32_t binding = static_cast<uint32_t>(layout_binding_lists[set_flag].size());
		VkShaderStageFlags real_stages = stages & pipeline->supported_shader_stages();
		layouts[set_flag]->add_binding(binding, descriptorType, real_stages, 1);

		// Push in binding_list£¬for later header file generation
		layout_binding_lists[set_flag].push_back(set_name + "_" + binding_name);


		// Record resource handle
		DescriptorHandle descriptor;
		descriptor.flags = set_flag;
		descriptor.binding = binding;
		descriptor.idx = 0;
		descriptor.type = descriptorType;
		descriptor.ptr = &tlas_list.back();

		descriptors.push_back(descriptor);
	}
}

void ResourceManager::register_resources(pstd::vector<Texture>&& textures, pstd::vector<Sampler>&& samplers, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages) {
	// Resources movement & Register searching list
	textures_lists.push_back(std::move(textures));
	samplers_lists.push_back(std::move(samplers));
	resources_searching_list.insert({ binding_name, &textures_lists.back() });


	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		// Create descriptor layout & Add binding 
		ResourceFlag set_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string set_name = rf_flag_to_string(set_flag);
		uint32_t set_num = (set_flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

		if (layouts.find(set_flag) == layouts.end()) {
			layouts.insert({ set_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

			layout_binding_lists.insert({ set_flag, pstd::vector<std::string>() });
		}

		uint32_t binding = static_cast<uint32_t>(layout_binding_lists[set_flag].size());
		VkShaderStageFlags real_stages = stages & pipeline->supported_shader_stages();
		layouts[set_flag]->add_binding(binding, descriptorType, real_stages, textures_lists.back().size());

		// Push in binding_list£¬for later header file generation
		layout_binding_lists[set_flag].push_back(set_name + "_" + binding_name);


		// Record resource handle
		DescriptorHandle descriptor;
		descriptor.flags = set_flag;
		descriptor.binding = binding;
		descriptor.idx = 0;
		descriptor.type = descriptorType;
		descriptor.ptr = &textures_lists.back();

		descriptors.push_back(descriptor);
	}
}

void ResourceManager::register_resources(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages, VkImageLayout targetLayout) {

	uint32_t resourceCount = (flags & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

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
		imagePtr->transition_layout(_context, cmdBuffer, targetLayout);
	}
	cmdBuffer.end_and_submit(_context.gc_queue(), true);

	for (const auto& pipeline : pipelines) {
		// Add pipeline ID
		pipeline_ids.insert(pipeline->get_id());

		ResourceFlag set_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
		std::string set_name = rf_flag_to_string(set_flag);
		uint32_t set_num = (set_flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

		if (layouts.find(set_flag) == layouts.end()) {
			layouts.insert({ set_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

			layout_binding_lists.insert({ set_flag, pstd::vector<std::string>() });
		}

		uint32_t binding = static_cast<uint32_t>(layout_binding_lists[set_flag].size());
		VkShaderStageFlags real_stages = stages & pipeline->supported_shader_stages();
		layouts[set_flag]->add_binding(binding, descriptorType, real_stages, 1);

		// Push in binding_list£¬for later header file generation
		layout_binding_lists[set_flag].push_back(set_name + "_" + binding_name);

		for (uint32_t i = 0; i < resourceCount; ++i) {
			DescriptorHandle descriptor;
			descriptor.flags = set_flag;
			descriptor.binding = binding;
			descriptor.idx = i;
			descriptor.type = descriptorType;
			descriptor.ptr = imagePtrs[i];

			descriptors.push_back(descriptor);

			// Check if resource is "window size related"
			if (flags & RF_WINDOW_SIZE_RELATED)
				window_size_related_resources_caches.push_back(descriptor);
		}
	}

}

// For creating Buffer inside the function
void ResourceManager::register_resources(VkDeviceSize size, VkDeviceSize stride, VkFormat format, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string binding_name, ResourceFlag flags, const pstd::vector<Pipeline*>& pipelines, VkDescriptorType descriptorType, VkShaderStageFlags stages) {
	uint32_t resourceCount = (flags & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

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
			ResourceFlag set_flag = rf_get_frequency(flags) | rf_get_pipeline(pipeline->get_id()) | RF_BIND_DESCRIPTOR;
			std::string set_name = rf_flag_to_string(set_flag);
			uint32_t set_num = (set_flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

			if (layouts.find(set_flag) == layouts.end()) {
				layouts.insert({ set_flag, &_context.descriptorManager().create_null_descriptor_set_layout(set_name, set_num) });

				layout_binding_lists.insert({ set_flag, pstd::vector<std::string>() });
			}

			uint32_t binding = static_cast<uint32_t>(layout_binding_lists[set_flag].size());
			VkShaderStageFlags real_stages = stages & pipeline->supported_shader_stages();
			layouts[set_flag]->add_binding(binding, descriptorType, real_stages, 1);

			// Push in binding_list£¬for later header file generation
			layout_binding_lists[set_flag].push_back(set_name + "_" + binding_name);

			for (uint32_t i = 0; i < resourceCount; ++i) {
				DescriptorHandle descriptor;
				descriptor.flags = set_flag;
				descriptor.binding = binding;
				descriptor.idx = i;
				descriptor.type = descriptorType;
				descriptor.ptr = bufferPtrs[i];

				descriptors.push_back(descriptor);

				// Check if resource is "window size related"
				if (flags & RF_WINDOW_SIZE_RELATED)
					window_size_related_resources_caches.push_back(descriptor);
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


void ResourceManager::build(const std::string& header_file_name) {
	// Init Descriptor Pool
	_context.descriptorManager().init_descriptor_pool();

	// Build Layout & Allocate Descriptor Set
	for (const auto& flag_layout : layouts) {
		// Build Layout
		flag_layout.second->build();

		ResourceFlag flag = flag_layout.first;
		uint32_t set_num = (flag & RF_PER_FRAME) ? MAX_FRAMES_IN_FLIGHT : 1;

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
	for (const auto& pipeline_id : pipeline_ids) {
		auto& pipeline = _context.pipelineManager().get(pipeline_id);
		pstd::vector<VkDescriptorSetLayout> tmp_layouts;

		ResourceFlag per_frame_layout_key = rf_get_frequency(RF_PER_FRAME) | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR;
		if (auto It = layouts.find(per_frame_layout_key); It != layouts.end())
			tmp_layouts.push_back(*It->second);

		ResourceFlag static_layout_key = rf_get_frequency(RF_STATIC) | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR;
		if (auto It = layouts.find(static_layout_key); It != layouts.end())
			tmp_layouts.push_back(*It->second);

		pipeline.register_descriptor_set_layout(tmp_layouts);
	}

	// Write In Descriptor 
	for (const auto& descriptor : descriptors) {
		ResourceFlag flag = descriptor.flags;
		bool if_multi = (flag & RF_PER_FRAME) && (MAX_FRAMES_IN_FLIGHT > 1);
		
		std::string set_name = rf_flag_to_string(flag);
		if (if_multi) set_name += std::to_string(descriptor.idx);

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

	// Generate Header file
	generate_header(header_file_name);
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

					ptr->transition_layout(_context, cmdBuffer,VK_IMAGE_LAYOUT_GENERAL);

					rebuilt_image.insert(ptr);
				}				
			}
			else if constexpr (std::is_same_v<T, Buffer>) {
				if (rebuilt_buffer.find(ptr) == rebuilt_buffer.end()) {
					// * ptr = _context.memAllocator().create_buffer()
					rebuilt_buffer.insert(ptr);
				}
			}
		}, handle.ptr);

		// Write in descriptor set
		std::string set_name = rf_flag_to_string(handle.flags);
		bool if_multi = (handle.flags & RF_PER_FRAME) && (MAX_FRAMES_IN_FLIGHT > 1);
		if (if_multi) set_name += std::to_string(handle.idx);

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

pstd::vector<VkDescriptorSet> ResourceManager::get_descriptor_sets(uint32_t pipeline_id, uint32_t frame_idx) {
	std::string static_set_name = rf_flag_to_string(rf_get_frequency(RF_STATIC) | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR) ;
	std::string dynamic_set_name = rf_flag_to_string(rf_get_frequency(RF_PER_FRAME) | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR);
	if (MAX_FRAMES_IN_FLIGHT > 1)dynamic_set_name += std::to_string(frame_idx);

	pstd::vector<VkDescriptorSet> pipeline_sets;
	if (auto It = sets.find(dynamic_set_name); It != sets.end())
		pipeline_sets.push_back(*It->second);
	if (auto It = sets.find(static_set_name); It != sets.end())
		pipeline_sets.push_back(*It->second);

	return pipeline_sets;
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

	return result;
}

ResourceFlag ResourceManager::rf_get_frequency(ResourceFlag flags) { return flags & RF_FREQUENCY_MASK; }

ResourceFlag ResourceManager::rf_get_bind_point(ResourceFlag flags) { return flags & RF_BINDPOINT_MASK; }

ResourceFlag ResourceManager::rf_get_pipeline(uint32_t pipeline_id) {
	ResourceFlag flag = static_cast<ResourceFlag>(pipeline_id);
	flag <<= 32;

	return flag;
}

void ResourceManager::generate_header(const std::string& filename) {
	std::ofstream out("./base/" + filename);
	if(!out.is_open())
		throw std::runtime_error("ResourceManager::Failed to open header file: " + filename);

	out << "// Auto-generated by ResourceManager, do not edit.\n";
	out << "#ifndef GENERATED_DESCRIPTOR_BINDINGS_H\n";
	out << "#define GENERATED_DESCRIPTOR_BINDINGS_H\n\n";

	// Descriptor Set
	pstd::vector<ResourceFlag> fr_flags{ RF_PER_FRAME, RF_STATIC };

	for (const auto& pipeline_id : pipeline_ids) {
		uint32_t set_count = 0;
		for (const auto& fr_flag : fr_flags) {
			ResourceFlag flag = rf_get_frequency(fr_flag) | rf_get_pipeline(pipeline_id) | RF_BIND_DESCRIPTOR;

			auto It = layout_binding_lists.find(flag);
			if (It == layout_binding_lists.end()) continue;

			const pstd::vector<std::string>& binding_names = It->second;
			std::string set_name = rf_flag_to_string(flag);

			out << "// " << set_name << "\n";
			out << "#define " << set_name << "_SET " << set_count << "\n";
			for (uint32_t i = 0; i < binding_names.size(); ++i) {
				std::string resource_binding = "BINDING_" + binding_names[i];
				for (auto& c : resource_binding) {
					if (c == ' ')c = '_';
					else c = static_cast<char>(std::toupper(c));
				}
				out << "#define " << resource_binding << " " << i << "\n";
			}

			set_count++;
			out << "\n\n";
		}
	}

	// Vertex Input
	if (!vertex_input_list.empty()) {
		out << "// Vertex Input\n";

		for (size_t i = 0; i < vertex_input_list.size(); ++i) {
			const auto& name = vertex_input_list[i];
			std::string macro_name = "VERTEX_LOCATION_" + name;
			for (auto& c : macro_name) {
				if (c == ' ')c = '_';
				else c = static_cast<char>(std::toupper(c));
			}
			out << "#define " << macro_name << " " << i << "\n";
		}
		out << "\n\n";
	}

	out << "#endif // GENERATED_DESCRIPTOR_BINDINGS_H\n";
	out.close();
}