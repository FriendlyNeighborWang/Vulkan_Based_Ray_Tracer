#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "vk_layer/Context.h"
#include "vk_layer/SwapChain.h"
#include "graphics/Window.h"
#include "graphics/SceneLoader.h"
#include "graphics/Renderer.h"



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

	VkPhysicalDeviceFeatures2 features{};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &asFeatures;

	auto validator = [&]() {
		return
			asFeatures.accelerationStructure &&
			rtFeatures.rayTracingPipeline &&
			scalarFeatures.scalarBlockLayout &&
			bufferDeviceAddressFeatures.bufferDeviceAddress;
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
	Scene scene = sceneLoader.LoadScene("resource/cornell_box/scene.gltf");
	


	// Build Acceleration Structure
	TLAS accelerationStructure = context.asManager().get_tlas(scene);

	// Import Shader 
	context.shaderManager().add_raygen_shader("./shader/raytrace.rgen.spv");
	context.shaderManager().add_miss_shader("./shader/raytrace.rmiss.spv");
	context.shaderManager().add_hit_group_shader("./shader/material_diffuse.rchit.spv");

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


	// Vertex Buffer & Index Buffer & Material Buffer & Geometry Buffer
	Buffer& vertexBuffer = scene.get_vertex_buffer(context.memAllocator());
	Buffer& indexBuffer = scene.get_index_buffer(context.memAllocator());
	Buffer& materialBuffer = scene.get_material_buffer(context.memAllocator());
	Buffer& geometryBuffer = scene.get_geometry_buffer(context.memAllocator());


	// Create Descriptor Set Layout
	
	DescriptorSetLayout& layout = context.descriptorManager().create_null_descriptor_set_layout();
	DescriptorSetLayout& compute_layout = context.descriptorManager().create_null_descriptor_set_layout();

	layout.add_binding(BINDING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	layout.add_binding(BINDING_TLAS, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	
	layout.add_binding(BINDING_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	layout.add_binding(BINDING_INDICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	layout.add_binding(BINDING_MATERIAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	layout.add_binding(BINDING_GEOMETRY, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	

	layout.build();

	compute_layout.add_binding(BINDING_COMPUTE_FORMAT_TRANSFER_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	compute_layout.add_binding(BINDING_COMPUTE_FORMAT_TRANSFER_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

	compute_layout.build();

	// PushConstant & Allocate Descriptor Set & Write Descriptor Set

	context.descriptorManager().init_descriptor_pool(2);
	DescriptorSet& rayTracingSet = context.descriptorManager().allocate_descriptor_set(0);
	DescriptorSet& computeSet = context.descriptorManager().allocate_descriptor_set(1);


	context.descriptorManager().descriptor_write(0, BINDING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, renderTarget);
	context.descriptorManager().descriptor_write(0, BINDING_TLAS, accelerationStructure);
	context.descriptorManager().descriptor_write(0, BINDING_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBuffer);
	context.descriptorManager().descriptor_write(0, BINDING_INDICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBuffer);
	context.descriptorManager().descriptor_write(0, BINDING_MATERIAL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, materialBuffer);
	context.descriptorManager().descriptor_write(0, BINDING_GEOMETRY, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, geometryBuffer);


	context.descriptorManager().descriptor_write(1, BINDING_COMPUTE_FORMAT_TRANSFER_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, renderTarget);
	context.descriptorManager().descriptor_write(1, BINDING_COMPUTE_FORMAT_TRANSFER_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, ldrImage);

	context.descriptorManager().update_descriptor_set();

	// Create Pipeline
	context.rtPipeline().create_pipeline(context.descriptorManager().get_descriptor_set_layouts({0}));

	ComputePipeline ftPipeline(context);
	ftPipeline.create_pipeline("./shader/format_trans.comp.spv", context.descriptorManager().get_descriptor_set_layouts({ 1 }), sizeof(FormatTransPushConstants));

	// Build Shader Binding Tbale & Get Shader Group Regions
	context.shaderManager().build_shader_binding_table(context.rtPipeline());


	Renderer renderer(context, window, swapchain);

	renderer.register_image("renderTarget", renderTarget);
	renderer.register_image("ldrImage", ldrImage);
	renderer.register_descriptor_set("rayTracingSet", rayTracingSet);
	renderer.register_descriptor_set("computeSet", computeSet);
	renderer.register_compute_pipeline("ftPipeline", ftPipeline);

	renderer.render();

	vkDeviceWaitIdle(context);


}
