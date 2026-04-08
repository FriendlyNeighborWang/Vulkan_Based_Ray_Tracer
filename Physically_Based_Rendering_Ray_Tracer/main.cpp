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
#include "graphics/SceneLoader.h"
#include "graphics/GLSLLayoutInfo.h"
#include "graphics/ResourceManager.h"
#include "graphics/Reservoir_struct.h"



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

	VkPhysicalDeviceDynamicRenderingFeatures dynRenderFeatures{};
	dynRenderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynRenderFeatures.pNext = &dIFeatures;

	VkPhysicalDeviceFeatures2 features{};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &dynRenderFeatures;

	auto validator = [&]() {
		return
			asFeatures.accelerationStructure &&
			rtFeatures.rayTracingPipeline &&
			scalarFeatures.scalarBlockLayout &&
			bufferDeviceAddressFeatures.bufferDeviceAddress &&
			dIFeatures.runtimeDescriptorArray &&
			dIFeatures.shaderSampledImageArrayNonUniformIndexing &&
			dynRenderFeatures.dynamicRendering;
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
	// Scene scene = sceneLoader.LoadScene("resource/Sponza/Sponza.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/Sponza_with_light/Sponza.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/cornell_box/scene.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/TransmissionTest/glTF/TransmissionTest.gltf");
	Scene scene = sceneLoader.LoadScene("resource/San_miguel/San_miguel.gltf");
	// Scene scene = sceneLoader.LoadScene("resource/Living_room/Living_room.gltf");


	scene.register_skybox("./resource/skybox/qwantani_morning_puresky_4k.hdr");

	// Create Renderer
	Renderer renderer(context, window, swapchain, scene);


	// Build Acceleration Structure
	TLAS accelerationStructure = context.asManager().get_tlas(scene);


	// Create Pipeline & Import Shader
	RTPipeline& rtPipeline = context.pipelineManager().create_rt_pipeline("RAY_TRACING");
	ComputePipeline& toneMappingPipeline = context.pipelineManager().create_compute_pipeline("TONE_MAPPING");
	RTPipeline& gBufferPipeline = context.pipelineManager().create_rt_pipeline("G_BUFFER");
	ComputePipeline& initializeReservoirPipeline = context.pipelineManager().create_compute_pipeline("INITIALIZE_RESERVOIR");
	ComputePipeline& temporalReusePipeline = context.pipelineManager().create_compute_pipeline("TEMPORAL_REUSE");
	ComputePipeline& spatialReusePipeline = context.pipelineManager().create_compute_pipeline("SPATIAL_REUSE");
	

	// Scene Data
	Buffer& vertexBuffer = scene.get_vertex_buffer(context);
	Buffer& indexBuffer = scene.get_index_buffer(context);
	Buffer& materialBuffer = scene.get_material_buffer(context);
	Buffer& geometryBuffer = scene.get_geometry_buffer(context);
	Buffer& lightBuffer = scene.get_light_buffer(context);
	Buffer& normalBuffer = scene.get_normal_buffer(context);
	Buffer& tangentBuffer = scene.get_tangent_buffer(context);
	Buffer& texcoordBuffer = scene.get_texcoord_buffer(context);
	auto textures_and_samplers= scene.get_textures_and_samplers(context);
	auto& textures = *pstd::get<0>(textures_and_samplers);
	auto& samplers = *pstd::get<1>(textures_and_samplers);
	auto skybox_textures_and_samplers = scene.get_skybox_textures_and_samplers(context);
	auto& skybox_textures = *pstd::get<0>(skybox_textures_and_samplers);
	auto& skybox_samplers = *pstd::get<1>(skybox_textures_and_samplers);


	// Scene Static Info
	Buffer& staticSceneInfoBuffer = scene.get_static_scene_info(context);


	// Register Resource
	ResourceManager& resourceManager = context.resourceManager();
	resourceManager.register_resources(std::move(staticSceneInfoBuffer), "SCENE_STATIC_INFO", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		{"", "SceneStaticInfo", "staticInfo", false });

	resourceManager.register_resources(std::move(vertexBuffer), "VERTEX_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "Vertex_Block", "vec3", "vertices", true });

	resourceManager.register_resources(std::move(indexBuffer), "INDEX_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "Index_Block", "uint", "indices", true });

	resourceManager.register_resources(std::move(materialBuffer), "MATERIAL_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "", "Material", "materials", true });

	resourceManager.register_resources(std::move(geometryBuffer), "GEOMETRY_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "", "GeometryStruct", "geometries", true });

	resourceManager.register_resources(std::move(lightBuffer), "LIGHT_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "", "Light", "lights", true });

	resourceManager.register_resources(std::move(normalBuffer), "NORMAL_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "Normal_Block", "vec3", "normals", true});

	resourceManager.register_resources(std::move(tangentBuffer), "TANGENT_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "Tangent_Block", "vec4", "tangents", true});

	resourceManager.register_resources(std::move(texcoordBuffer), "TEXCOORD_BUFFER", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "Texcoord_Block", "vec2", "texcoords", true});

	resourceManager.register_resources(std::move(textures), std::move(samplers), "TEXTURE_ARRAY", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		{ "", "", "textures", true });

	resourceManager.register_resources(std::move(accelerationStructure), "ACCELERATION_STRUCTURE", RF_STATIC | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline }, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		{ "", "", "tlas", false});

	resourceManager.register_resources(std::move(skybox_textures), std::move(skybox_samplers), "SKYBOX_TEXTURE_ARRAY", RF_STATIC | RF_BIND_DESCRIPTOR, { &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline, &rtPipeline }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		{ "", "", "skyboxTextures", true });


	resourceManager.register_resources(static_cast<VkDeviceSize>(sizeof(Scene::SceneDynamicInfo)), sizeof(Scene::SceneDynamicInfo), VK_FORMAT_UNDEFINED, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "DYNAMIC_INFO", RF_PER_FRAME | RF_BIND_DESCRIPTOR, { &rtPipeline, &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline }, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		{ "", "SceneDynamicInfo", "dynamicInfo", false });


	// Render Target
	resourceManager.register_resources(swapchain.getExtent(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "HDR_IMAGE", RF_PER_FRAME | RF_BIND_DESCRIPTOR | RF_WINDOW_SIZE_RELATED, { &rtPipeline, &toneMappingPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL,
		{ "", "", "hdrImage", false });

	resourceManager.register_resources(swapchain.getExtent(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "LDR_IMAGE", RF_PER_FRAME | RF_BIND_DESCRIPTOR | RF_WINDOW_SIZE_RELATED, { &toneMappingPipeline }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL,
		{ "", "", "ldrImage", false });


	// Reservoir Buffer
	resourceManager.register_resources(
		swapchain.getArea()* static_cast<VkDeviceSize>(sizeof(Reservoir)),
		static_cast<VkDeviceSize>(sizeof(Reservoir)),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		"RESERVOIR_BUFFER",
		RF_TEMPORAL | RF_BIND_DESCRIPTOR | RF_WINDOW_SIZE_RELATED,
		{ &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline, &rtPipeline },
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "Reservoir_Buffer", "Reservoir", "reservoirs", true }
	);

	// G-Buffer
	resourceManager.register_resources(
		swapchain.getArea()* static_cast<VkDeviceSize>(sizeof(GBufferElement)),
		static_cast<VkDeviceSize>(sizeof(GBufferElement)),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		"G_BUFFER",
		RF_TEMPORAL | RF_BIND_DESCRIPTOR | RF_WINDOW_SIZE_RELATED,
		{ &gBufferPipeline, &initializeReservoirPipeline, &temporalReusePipeline, &spatialReusePipeline, &rtPipeline },
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		{ "G_Buffer", "GBufferElement", "gBuffer", true }
	);
		
	
	


	resourceManager.build();

	// Register Shaders¡¢
	
	gBufferPipeline.register_raygen_shader("./shader/gBuffer/gBuffer.rgen");
	gBufferPipeline.register_miss_shader("./shader/gBuffer/gBuffer.rmiss");
	gBufferPipeline.register_hit_group_shader("./shader/gBuffer/gBuffer.rchit", "./shader/gBuffer/gBuffer.rahit");

	initializeReservoirPipeline.register_compute_shader("./shader/reservoir/init_reservoir.comp");
	temporalReusePipeline.register_compute_shader("./shader/reservoir/temporal_reuse.comp");
	spatialReusePipeline.register_compute_shader("./shader/reservoir/spatial_reuse.comp");

	// rtPipeline.register_raygen_shader("./shader/rayGen.rgen");
	rtPipeline.register_raygen_shader("./shader/ReSTIR.rgen");
	rtPipeline.register_miss_shader("./shader/rayMiss.rmiss");
	rtPipeline.register_miss_shader("./shader/shadowRayMiss.rmiss");
	rtPipeline.register_hit_group_shader("./shader/closestHit.rchit", "./shader/alphaTest.rahit");
	rtPipeline.register_hit_group_shader("./shader/shadowRayHit.rchit", "./shader/shadowRayAnyHit.rahit");

	toneMappingPipeline.register_compute_shader("./shader/tone_mapping.comp");

	
	// Build Pipeline
	gBufferPipeline.build(context, sizeof(PushConstants));
	initializeReservoirPipeline.build(context, sizeof(ReservoirPushConstants));
	temporalReusePipeline.build(context, sizeof(ReservoirPushConstants));
	spatialReusePipeline.build(context, sizeof(ReservoirPushConstants));
	rtPipeline.build(context, sizeof(PushConstants));
	toneMappingPipeline.build(context, sizeof(ToneMappingPushConstants));




	// renderer.offline_render("result.hdr");
	renderer.realtime_render();

	vkDeviceWaitIdle(context);

}
