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
#include "graphics/SkyBox.h"



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
	// Scene scene = sceneLoader.LoadScene("resource/Sponza_with_light/Sponza.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/cornell_box/scene.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/TransmissionTest/glTF/TransmissionTest.gltf");

	SkyBox skybox(context, "./resource/skybox/citrus_orchard_road_puresky_4k.hdr");
	// SkyBox skybox(context);

	// Create Renderer
	Renderer renderer(context, window, swapchain, scene);


	// Build Acceleration Structure
	TLAS accelerationStructure = context.asManager().get_tlas(scene);


	// Import Shader 
	context.shaderManager().add_raygen_shader("./shader/spv/rayGen.rgen.spv");
	context.shaderManager().add_miss_shader("./shader/spv/rayMiss.rmiss.spv");
	context.shaderManager().add_miss_shader("./shader/spv/shadowRayMiss.rmiss.spv");
	context.shaderManager().add_hit_group_shader("./shader/spv/closestHit.rchit.spv", "./shader/spv/alphaTest.rahit.spv");
	context.shaderManager().add_hit_group_shader("./shader/spv/shadowRayHit.rchit.spv", "./shader/spv/shadowRayAnyHit.rahit.spv");

	context.shaderManager().build_shader_stages_and_shader_groups();

	// Create Descriptor Set & Resource
	// Allocate Image & Image Layout Transition to GENERAL
	renderer.create_images();
	const auto& hdrImages = renderer.get_hdrImages();
	const auto& ldrImages = renderer.get_ldrImages();

	renderer.create_uniform_buffer();
	const auto& dynamicInfoBuffers = renderer.get_uniform_buffers();
	

	// Scene Data
	Buffer& vertexBuffer = scene.get_vertex_buffer(context);
	Buffer& indexBuffer = scene.get_index_buffer(context);
	Buffer& materialBuffer = scene.get_material_buffer(context);
	Buffer& geometryBuffer = scene.get_geometry_buffer(context);
	Buffer& lightBuffer = scene.get_light_buffer(context, skybox);
	Buffer& normalBuffer = scene.get_normal_buffer(context);
	Buffer& tangentBuffer = scene.get_tangent_buffer(context);
	Buffer& texcoordBuffer = scene.get_texcoord_buffer(context);
	pstd::vector<Texture>& textures = scene.get_textures(context);

	// Scene Static Info
	Buffer& staticSceneInfoBuffer = scene.get_static_scene_info(context);

	const pstd::vector<Texture>& skyboxTextures = skybox.get_skybox_textures();


	// Create Descriptor Set Layout
	
	DescriptorSetLayout& rt_dynamic_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_DYNAMIC_SET_LAYOUT");
	DescriptorSetLayout& rt_image_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_IMAGE_SET_LAYOUT");
	DescriptorSetLayout& rt_uniform_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_UNIFORM_SET_LAYOUT");
	DescriptorSetLayout& rt_skybox_layout = context.descriptorManager().create_null_descriptor_set_layout("RAY_TRACING_SKYBOX_LAYOUT");
	DescriptorSetLayout& compute_tone_mapping_layout = context.descriptorManager().create_null_descriptor_set_layout("COMPUTE_TONE_MAPPING_SET_LAYOUT");
	

	rt_dynamic_layout.add_binding(BINDING_RAY_TRACING_SCENE_DYNAMIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	rt_dynamic_layout.build();


	rt_image_layout.add_binding(BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	rt_image_layout.build();


	rt_uniform_layout.add_binding(BINDING_RAY_TRACING_SCENE_STATIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
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

	
	rt_skybox_layout.add_binding(BINDING_RAY_TRACING_SKYBOX_TEXTURE_ARRAY, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, static_cast<uint32_t>(skyboxTextures.size()));

	rt_skybox_layout.build();


	compute_tone_mapping_layout.add_binding(BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	compute_tone_mapping_layout.add_binding(BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

	compute_tone_mapping_layout.build();

	// Allocate Descriptor Set & Write Descriptor Set

	context.descriptorManager().init_descriptor_pool(2 + 3 * MAX_FRAMES_IN_FLIGHT);
	DescriptorSet& rtUniformSet = context.descriptorManager().allocate_descriptor_set("RAY_TRACING_UNIFORM_SET_LAYOUT");
	DescriptorSet& rtSkyBoxSet = context.descriptorManager().allocate_descriptor_set("RAY_TRACING_SKYBOX_LAYOUT");
	pstd::vector<DescriptorSet*> rtDynamicSets(MAX_FRAMES_IN_FLIGHT);
	pstd::vector<DescriptorSet*> rtImageSets(MAX_FRAMES_IN_FLIGHT);
	pstd::vector<DescriptorSet*> toneMappingSets(MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		rtDynamicSets[i] = &context.descriptorManager().allocate_descriptor_set("RAY_TRACING_DYNAMIC_SET_LAYOUT");
		rtImageSets[i] = &context.descriptorManager().allocate_descriptor_set("RAY_TRACING_IMAGE_SET_LAYOUT");
		toneMappingSets[i] = &context.descriptorManager().allocate_descriptor_set("COMPUTE_TONE_MAPPING_SET_LAYOUT");
	}


	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_SCENE_STATIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, staticSceneInfoBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_TLAS, accelerationStructure);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_INDICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_MATERIAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, materialBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_GEOMETRY, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, geometryBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_LIGHT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, lightBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_TEXCOORD0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, texcoordBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_TEXTURE_ARRAY, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textures);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_NORMAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, normalBuffer);
	rtUniformSet.descriptor_write(BINDING_RAY_TRACING_TANGENT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, tangentBuffer);

	

	rtSkyBoxSet.descriptor_write(BINDING_RAY_TRACING_SKYBOX_TEXTURE_ARRAY, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, skyboxTextures);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		rtDynamicSets[i]->descriptor_write(BINDING_RAY_TRACING_SCENE_DYNAMIC_INFO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dynamicInfoBuffers[i]);

		rtImageSets[i]->descriptor_write(BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, hdrImages[i]);

		toneMappingSets[i]->descriptor_write(BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, hdrImages[i]);
		toneMappingSets[i]->descriptor_write(BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, ldrImages[i]);
	}


	context.descriptorManager().update_descriptor_set();



	// Create Pipeline (Order is important)
	context.rtPipeline().create_pipeline(context.descriptorManager().get_descriptor_set_layouts({ "RAY_TRACING_DYNAMIC_SET_LAYOUT", "RAY_TRACING_IMAGE_SET_LAYOUT", "RAY_TRACING_UNIFORM_SET_LAYOUT", "RAY_TRACING_SKYBOX_LAYOUT"}));

	ComputePipeline tmPipeline(context);
	tmPipeline.create_pipeline("./shader/spv/tone_mapping.comp.spv", context.descriptorManager().get_descriptor_set_layouts({ "COMPUTE_TONE_MAPPING_SET_LAYOUT" }), sizeof(ToneMappingPushConstants));

	// Build Shader Binding Table
	context.shaderManager().build_shader_binding_table(context.rtPipeline());


	// Register Resource 
	renderer.register_descriptor_set("rtUniformSet", rtUniformSet);
	renderer.register_descriptor_set("rtSkyBoxSet", rtSkyBoxSet);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		renderer.register_descriptor_set("rtDynamicSet" + std::to_string(i), *rtDynamicSets[i]);
		renderer.register_descriptor_set("rtImageSet" + std::to_string(i), *rtImageSets[i]);
		renderer.register_descriptor_set("computeToneMappingSet" + std::to_string(i), *toneMappingSets[i]);
	}
	renderer.register_compute_pipeline("tmPipeline", tmPipeline);
	

	// renderer.offline_render("result.hdr");
	renderer.realtime_render();

	vkDeviceWaitIdle(context);


}
