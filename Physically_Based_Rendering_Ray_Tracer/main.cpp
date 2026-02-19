#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>


#include "vk_layer/Image.h"
#include "vk_layer/Context.h"
#include "vk_layer/SwapChain.h"
#include "vk_layer/ASManager.h"
#include "vk_layer/SyncObject.h"
#include "vk_layer/RTPipeline.h"
#include "vk_layer/CommandPool.h"
#include "vk_layer/ComputePipeline.h"
#include "vk_layer/DescriptorManager.h"
#include "vk_layer/VkMemoryAllocator.h"

#include "graphics/Window.h"
#include "graphics/Renderer.h"
#include "graphics/SceneLoader.h"



int main() {
	Context context;
	// Instance
	context.enable_validation_layers();
	context.add_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	Window window(WIDTH, HEIGHT);
	auto extensionsForWindow = Window::glfw_extension();
	for (const auto& extension : extensionsForWindow)
		context.add_instance_extension(extension);

	context.create_instance();

	// Surface
	context.create_surface(window);

	// Physical Device
	pstd::vector<const char*> rayTracingExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
	};
	for (const auto& extension : rayTracingExtensions)
		context.add_device_extension(extension);


	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
	rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rtFeatures.pNext = &bufferDeviceAddressFeatures;

	VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures{};
	scalarFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
	scalarFeatures.pNext = &rtFeatures;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
	asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	asFeatures.pNext = &scalarFeatures;

	VkPhysicalDeviceDescriptorIndexingFeatures dIFeatures{};
	dIFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	dIFeatures.pNext = &asFeatures;

	VkPhysicalDeviceFeatures2 features{};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &dIFeatures;

	auto validator = [&]() {
		return
			asFeatures.accelerationStructure &&
			rtFeatures.rayTracingPipeline &&
			scalarFeatures.scalarBlockLayout &&
			bufferDeviceAddressFeatures.bufferDeviceAddress &&
			dIFeatures.runtimeDescriptorArray &&
			dIFeatures.shaderSampledImageArrayNonUniformIndexing;
		};
	context.register_device_feature(features, validator);

	context.pick_physical_device();

	// Create Device
	context.create_device();

	// Initialize
	context.init();




	// Create SwapChain
	SwapChain swapchain(context, window);

	// Load Scene
	SceneLoader& sceneLoader = SceneLoader::Get();
	Scene scene = sceneLoader.LoadScene("resource/Sponza/Sponza.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/cornell_box/scene.gltf");
	


	// Build Acceleration Structure
	TLAS accelerationStructure = context.asManager().get_tlas(scene);

	// Import Shader 
	context.shaderManager().add_raygen_shader("./shader/spv/rayGen.rgen.spv");
	context.shaderManager().add_miss_shader("./shader/spv/rayMiss.rmiss.spv");
	context.shaderManager().add_miss_shader("./shader/spv/shadowRayMiss.rmiss.spv");
	context.shaderManager().add_hit_group_shader("./shader/spv/closestHit.rchit.spv", "./shader/spv/alphaTest.rahit.spv");
	context.shaderManager().add_hit_group_shader("./shader/spv/shadowRayHit.rchit.spv", "./shader/spv/alphaTest.rahit.spv");
	

	context.shaderManager().build_shader_stages_and_shader_groups();

	// Create Descriptor Set & Resource
	// Allocate Image & Image Layout Transition to GENERAL

	Image renderTarget = context.memAllocator().create_image(
		swapchain.getExtent(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	Image ldrImage = context.memAllocator().create_image(
		swapchain.getExtent(),
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	{
		CommandBuffer cmdBuffer = context.cmdPool().get_command_buffer();
		cmdBuffer.begin();
		renderTarget.transition_layout(
			context, cmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);

		ldrImage.transition_layout(
			context, cmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		cmdBuffer.end_and_submit(context.gc_queue(), true);
	}

	// Scene Dynamic & Static Info
	Buffer& dynamicSceneInfoBuffer = scene.get_dynamic_scene_info(context);
	Buffer& staticSceneInfoBuffer = scene.get_static_scene_info(context);

	// Scene Data
	Buffer& vertexBuffer = scene.get_vertex_buffer(context);
	Buffer& indexBuffer = scene.get_index_buffer(context);
	Buffer& materialBuffer = scene.get_material_buffer(context);
	Buffer& geometryBuffer = scene.get_geometry_buffer(context);
	Buffer& lightBuffer = scene.get_light_buffer(context);
	Buffer& normalBuffer = scene.get_normal_buffer(context);
	Buffer& tangentBuffer = scene.get_tangent_buffer(context);
	Buffer& texcoordBuffer = scene.get_texcoord_buffer(context);
	pstd::vector<Texture>& textures = scene.get_textures(context);


	// Create Descriptor Set Layout
	
	DescriptorSetLayout& rt_dynamic_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_DYNAMIC_SET_LAYOUT");
	DescriptorSetLayout& rt_image_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_IMAGE_SET_LAYOUT");
	DescriptorSetLayout& rt_uniform_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_UNIFORM_SET_LAYOUT");
	DescriptorSetLayout& compute_tone_mapping_layout = context.descriptorManager().create_null_descriptor_set_layout("COMPUTE_TONE_MAPPING_SET_LAYOUT");

	rt_dynamic_layout.add_binding(BINDING_RAY_TRACING_SCENE_DYNAMIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	rt_dynamic_layout.build();


	rt_image_layout.add_binding(BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	rt_image_layout.build();


	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_SCENE_STATIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_TLAS, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_INDICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_MATERIAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_GEOMETRY, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_LIGHT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_TEXCOORD0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_TEXTURE_ARRAY, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, static_cast<uint32_t>(textures.size()));
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_NORMAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_TANGENT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);

	rt_uniform_layout.build();


	compute_tone_mapping_layout.add_binding(BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	compute_tone_mapping_layout.add_binding(BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

	compute_tone_mapping_layout.build();

	// PushConstant & Allocate Descriptor Set & Write Descriptor Set

	context.descriptorManager().init_descriptor_pool(4);
	DescriptorSet& rtDynamicSet = context.descriptorManager().allocate_descriptor_set("RAY_TRACING_DYNAMIC_SET_LAYOUT", "RAY_TRACING_DYNAMIC_SET");
	DescriptorSet& rtImageSet = context.descriptorManager().allocate_descriptor_set("RAY_TRACING_IMAGE_SET_LAYOUT", "RAY_TRACING_IMAGE_SET");
	DescriptorSet& rtUniformSet = context.descriptorManager().allocate_descriptor_set("RAY_TRACING_UNIFORM_SET_LAYOUT", "RAY_TRACING_UNIFORM_SET");
	DescriptorSet& computeToneMappingSet = context.descriptorManager().allocate_descriptor_set("COMPUTE_TONE_MAPPING_SET_LAYOUT","COMPUTE_TONE_MAPPING_SET");


	context.descriptorManager().descriptor_write("RAY_TRACING_DYNAMIC_SET", BINDING_RAY_TRACING_SCENE_DYNAMIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dynamicSceneInfoBuffer);


	context.descriptorManager().descriptor_write("RAY_TRACING_IMAGE_SET", BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, renderTarget);


	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_SCENE_STATIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, staticSceneInfoBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_TLAS, accelerationStructure);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_INDICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_MATERIAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, materialBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_GEOMETRY, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, geometryBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_LIGHT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, lightBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_TEXCOORD0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, texcoordBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_TEXTURE_ARRAY, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textures);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_NORMAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, normalBuffer);
	context.descriptorManager().descriptor_write("RAY_TRACING_UNIFORM_SET", BINDING_RAY_TRACING_TANGENT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, tangentBuffer);


	context.descriptorManager().descriptor_write("COMPUTE_TONE_MAPPING_SET", BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, renderTarget);
	context.descriptorManager().descriptor_write("COMPUTE_TONE_MAPPING_SET", BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, ldrImage);

	context.descriptorManager().update_descriptor_set();

	// Create Pipeline
	context.rtPipeline().create_pipeline(context.descriptorManager().get_descriptor_set_layouts({ "RAY_TRACING_DYNAMIC_SET_LAYOUT", "RAY_TRACING_IMAGE_SET_LAYOUT", "RAY_TRACING_UNIFORM_SET_LAYOUT"}));

	ComputePipeline tmPipeline(context);
	tmPipeline.create_pipeline("./shader/spv/tone_mapping.comp.spv", context.descriptorManager().get_descriptor_set_layouts({ "COMPUTE_TONE_MAPPING_SET_LAYOUT" }), sizeof(ToneMappingPushConstants));

	// Build Shader Binding Tbale & Get Shader Group Regions
	context.shaderManager().build_shader_binding_table(context.rtPipeline());


	Renderer renderer(context, window, swapchain, scene);

	renderer.register_image("renderTarget", renderTarget);
	renderer.register_image("ldrImage", ldrImage);
	renderer.register_descriptor_set("rtImageSet", rtImageSet);
	renderer.register_descriptor_set("rtDynamicSet", rtDynamicSet);
	renderer.register_descriptor_set("rtUniformSet", rtUniformSet);
	renderer.register_descriptor_set("computeToneMappingSet", computeToneMappingSet);
	renderer.register_compute_pipeline("tmPipeline", tmPipeline);
	

	// renderer.offline_render("result.hdr");
	
	renderer.realtime_render();

	vkDeviceWaitIdle(context);


}
