#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>


#include "vk_layer/Image.h"
#include "vk_layer/Context.h"
#include "vk_layer/Pipeline.h"
#include "vk_layer/SwapChain.h"
#include "vk_layer/ASManager.h"
#include "vk_layer/SyncObject.h"
#include "vk_layer/CommandPool.h"
#include "vk_layer/PipelineManager.h"
#include "vk_layer/DescriptorManager.h"
#include "vk_layer/VkMemoryAllocator.h"

#include "graphics/Window.h"
#include "graphics/SkyBox.h"
#include "graphics/Renderer.h"
#include "graphics/RenderPass.h"
#include "graphics/SceneLoader.h"
#include "graphics/ResourceManager.h"



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

	SkyBox skybox("./resource/skybox/citrus_orchard_road_puresky_4k.hdr");
	// SkyBox skybox();

	// Create Renderer
	Renderer renderer(context, window, swapchain, scene);


	// Build Acceleration Structure
	TLAS accelerationStructure = context.asManager().get_tlas(scene);


	// Create Pipeline & Import Shader
	RTPipeline& rtPipeline = context.pipelineManager().create_rt_pipeline("RAY_TRACING");
	ComputePipeline& toneMappingPipeline = context.pipelineManager().create_compute_pipeline("TONE_MAPPING");
	

	// Scene Data
	Buffer& vertexBuffer = scene.get_vertex_buffer(context);
	Buffer& indexBuffer = scene.get_index_buffer(context);
	Buffer& materialBuffer = scene.get_material_buffer(context);
	Buffer& geometryBuffer = scene.get_geometry_buffer(context);
	Buffer& lightBuffer = scene.get_light_buffer(context, skybox);
	Buffer& normalBuffer = scene.get_normal_buffer(context);
	Buffer& tangentBuffer = scene.get_tangent_buffer(context);
	Buffer& texcoordBuffer = scene.get_texcoord_buffer(context);
	auto textures_and_samplers= scene.get_textures_and_samplers(context);
	auto& textures = *pstd::get<0>(textures_and_samplers);
	auto& samplers = *pstd::get<1>(textures_and_samplers);
	auto skybox_textures_and_samplers = skybox.get_skybox_textures_and_samplers(context);
	auto& skybox_textures = *pstd::get<0>(skybox_textures_and_samplers);
	auto& skybox_samplers = *pstd::get<1>(skybox_textures_and_samplers);


	// Scene Static Info
	Buffer& staticSceneInfoBuffer = scene.get_static_scene_info(context);
	
	// RenderPass
	// RenderPass rayTracingPass("RAY_TRACING", RenderPass::PassType::RAY_TRACING);
	// RenderPass toneMappingPass("TONE_MAPPING", RenderPass::PassType::COMPUTE);


	ResourceManager& resourceManager = context.resourceManager();
	resourceManager.register_resources(std::move(staticSceneInfoBuffer), "SCENE_STATIC_INFO", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
	resourceManager.register_resources(std::move(vertexBuffer), "VERTEX_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(indexBuffer), "INDEX_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(materialBuffer), "MATERIAL_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(geometryBuffer), "GEOMETRY_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(lightBuffer), "LIGHT_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(normalBuffer), "NORMAL_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(tangentBuffer), "TANGENT_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(texcoordBuffer), "TEXCOORD_BUFFER", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(textures), std::move(samplers), "TEXTURE_ARRAY", RF_STATIC, { &rtPipeline },
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	resourceManager.register_resources(std::move(accelerationStructure), "ACCELERATION_STRUCTURE", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	resourceManager.register_resources(std::move(skybox_textures), std::move(skybox_samplers), "SKYBOX_TEXTURE_ARRAY", RF_STATIC, { &rtPipeline }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);


	resourceManager.register_resources(static_cast<VkDeviceSize>(sizeof(Scene::SceneDynamicInfo)), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "DYNAMIC_INFO", RF_PER_FRAME, { &rtPipeline }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR);


	resourceManager.register_resources(swapchain.getExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "HDR_IMAGE", RF_PER_FRAME | RF_WINDOW_SIZE_RELATED, { &rtPipeline, &toneMappingPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT, VK_IMAGE_LAYOUT_GENERAL);
	resourceManager.register_resources(swapchain.getExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "LDR_IMAGE", RF_PER_FRAME | RF_WINDOW_SIZE_RELATED, { &toneMappingPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, VK_IMAGE_LAYOUT_GENERAL);


	resourceManager.build();

	// Register Shaders

	rtPipeline.register_raygen_shader("./shader/rayGen.rgen");
	rtPipeline.register_miss_shader("./shader/rayMiss.rmiss");
	rtPipeline.register_miss_shader("./shader/shadowRayMiss.rmiss");
	rtPipeline.register_hit_group_shader("./shader/closestHit.rchit", "./shader/alphaTest.rahit");
	rtPipeline.register_hit_group_shader("./shader/shadowRayHit.rchit", "./shader/shadowRayAnyHit.rahit");

	toneMappingPipeline.register_compute_shader("./shader/tone_mapping.comp");

	rtPipeline.build(context, sizeof(PushConstants));
	toneMappingPipeline.build(context, sizeof(ToneMappingPushConstants));


	// Render
	renderer.prepare_frame_context();

	// renderer.offline_render("result.hdr");
	renderer.realtime_render();

	vkDeviceWaitIdle(context);


}
