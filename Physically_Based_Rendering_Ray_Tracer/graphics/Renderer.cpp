#include "Renderer.h"


#include "Scene.h"
#include "vk_layer/Image.h"
#include "vk_layer/Context.h"
#include "vk_layer/SwapChain.h"
#include "vk_layer/SyncObject.h"
#include "vk_layer/RTPipeline.h"
#include "vk_layer/CommandPool.h"
#include "vk_layer/ComputePipeline.h"
#include "vk_layer/DescriptorManager.h"
#include "vk_layer/VkMemoryAllocator.h"

#include "stb_image_write.h"

Renderer::Renderer(Context& context, Window& window, SwapChain& swapChain, Scene& scene) :_context(context), window(window), swapChain(swapChain), scene(scene), cameraController(scene.dynamicInfo.camera){
	// Load Dynamic Function
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(context, "vkCmdTraceRaysKHR"));
}



void Renderer::register_descriptor_set(std::string name, DescriptorSet& descriptorSet) {
	descriptorSets.insert({ name, &descriptorSet });
}

void Renderer::register_compute_pipeline(std::string name, ComputePipeline& pipeline) {
	computePipelines.insert({ name, &pipeline });
}

void Renderer::create_uniform_buffer() {
	uniformBuffers.clear();

	uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		uniformBuffers.push_back(scene.get_dynamic_scene_info(_context));
	}
}

void Renderer::create_images() {
	hdrImages.clear();
	ldrImages.clear();

	hdrImages.reserve(MAX_FRAMES_IN_FLIGHT);
	ldrImages.reserve(MAX_FRAMES_IN_FLIGHT);


	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin();

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		Image renderTarget = _context.memAllocator().create_image(
			swapChain.getExtent(),
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		Image ldrImage = _context.memAllocator().create_image(
			swapChain.getExtent(),
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		renderTarget.transition_layout(
			_context, cmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);

		ldrImage.transition_layout(
			_context, cmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);


		hdrImages.push_back(std::move(renderTarget));
		ldrImages.push_back(std::move(ldrImage));
	}


	cmdBuffer.end_and_submit(_context.gc_queue(), true);
}


void Renderer::recreateSwapChain() {
	swapChain.recreate(window);
	
	// Image Recreate & Layout Transtion

	create_images();

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		imageSets[i]->descriptor_write(BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, hdrImages[i]);

		toneMappingSets[i]->descriptor_write(BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, hdrImages[i]);
		toneMappingSets[i]->descriptor_write(BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, ldrImages[i]);
	}

	
	_context.descriptorManager().update_descriptor_set();
}

void Renderer::updateDynamicSceneInfo(Timer& timer) {
	window.process_input();

	auto& dynamic_info = scene.dynamicInfo;
	// update Data
	cameraController.update(window, timer.get_delta());

	// Write into Uniform Buffer
	Buffer& dynamic_info_buffer = uniformBuffers[currentFrame];

	dynamic_info_buffer.write_buffer(&dynamic_info.camera, sizeof(dynamic_info.camera), scene.dynamicInfo.camera_data_offset());

}

void Renderer::updateRenderingSetting(Scene::SceneDynamicInfo dynamicInfo) {
	for (auto& dynamic_info_buffer : uniformBuffers) {
		dynamic_info_buffer.write_buffer(&dynamicInfo, sizeof(dynamicInfo));
	}
}

void Renderer::realtime_render() {
	// Get Shader Groups Region
	groupRegions = _context.shaderManager().get_shader_group_regions();

	// Resources Set
	imageSets.reserve(MAX_FRAMES_IN_FLIGHT);
	toneMappingSets.reserve(MAX_FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		dynamicSets.push_back(descriptorSets.find("rtDynamicSet" + std::to_string(i))->second);
		imageSets.push_back(descriptorSets.find("rtImageSet" + std::to_string(i))->second);
		toneMappingSets.push_back(descriptorSets.find("computeToneMappingSet" + std::to_string(i))->second);
	}

	VkDescriptorSet rtUniformSet = *(descriptorSets.find("rtUniformSet")->second);
	VkDescriptorSet rtSkyBoxSet = *(descriptorSets.find("rtSkyBoxSet")->second);
	
	pstd::vector<VkDescriptorSet> render_sets(4);
	render_sets[RAY_TRACING_DYNAMIC_SET] = dynamicSets[0]->get();
	render_sets[RAY_TRACING_IMAGE_SET] = imageSets[0]->get();
	render_sets[RAY_TRACING_UNIFORM_SET] = rtUniformSet;
	render_sets[RAY_TRACING_SKYBOX_SET] = rtSkyBoxSet;
	pstd::vector<VkDescriptorSet> tone_mapping_sets = { toneMappingSets[0]->get() };

	PushConstants pushConstants;
	ToneMappingPushConstants tmPushConstants;

	ComputePipeline& ftPipeline = *(computePipelines.find("tmPipeline")->second);

	// Timer
	Timer& generalTimer = timerManager.register_timer("General");
	Timer& deltaTimer = timerManager.register_timer("Delta");

	// CommandBuffer
	while (_context.cmdPool().available_num() < MAX_FRAMES_IN_FLIGHT + swapChain.imageCount())
		_context.cmdPool().allocate_command_buffer();

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		renderCmdBuffers.push_back(std::move(_context.cmdPool().get_command_buffer()));
	}


	// Sync Object
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		inFlightFences.emplace_back(_context, true);
		imageAvailableSemaphores.emplace_back(_context);
	}

	for (uint32_t i = 0; i < swapChain.imageCount(); ++i) {
		renderFinishedSemaphores.emplace_back(_context);
	}

	
	// Set Renderring param
	auto& renderingSeting = scene.dynamicInfo;
	renderingSeting.iteration_depth = 4;
	renderingSeting.samples_per_pixel = 8;
	updateRenderingSetting(renderingSeting);

	// Render
	while (!window.should_close()) {
		window.poll_events();

		inFlightFences[currentFrame].wait();
		

		// Resource Setting & Acquire Next Image
		CommandBuffer& renderCmdBuffer = renderCmdBuffers[currentFrame];
		Image& renderTarget = hdrImages[currentFrame];
		Image& ldrImage = ldrImages[currentFrame];
		Semaphore& imageAvaiableSemaphore = imageAvailableSemaphores[currentFrame];

		uint32_t imageIndex;
		VkResult result = swapChain.acquire_next_image(imageAvaiableSemaphore, &imageIndex);

		Semaphore& renderFinishedSemaphore = renderFinishedSemaphores[imageIndex];

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			continue;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Renderer::Failed to acquire swap chain image");

		inFlightFences[currentFrame].reset();



		// Start Timing
		generalTimer.start();


		// Render Pass

		renderCmdBuffer.begin();

		// Bind Pipeline & Descriptor Set
		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _context.rtPipeline());

		updateDynamicSceneInfo(deltaTimer);
		render_sets[RAY_TRACING_DYNAMIC_SET] = dynamicSets[currentFrame]->get();
		render_sets[RAY_TRACING_IMAGE_SET] = imageSets[currentFrame]->get();

		vkCmdBindDescriptorSets(
			renderCmdBuffer,
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			_context.rtPipeline().pipeline_layout(),
			0,
			render_sets.size(), render_sets.data(),
			0, nullptr
		);
		

		pushConstants.sample_batch = 0;

		vkCmdPushConstants(
			renderCmdBuffer,
			_context.rtPipeline().pipeline_layout(),
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			0, sizeof(pushConstants),
			&pushConstants
		);


		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ftPipeline);

		tone_mapping_sets[COMPUTE_TONE_MAPPING_SET] = toneMappingSets[currentFrame]->get();

		vkCmdBindDescriptorSets(
			renderCmdBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			ftPipeline.pipeline_layout(),
			0,
			tone_mapping_sets.size(), tone_mapping_sets.data(),
			0, nullptr
		);

		tmPushConstants.exposure = 0.5f;

		vkCmdPushConstants(
			renderCmdBuffer,
			ftPipeline.pipeline_layout(),
			VK_SHADER_STAGE_COMPUTE_BIT,
			0, sizeof(tmPushConstants),
			&tmPushConstants
		);

		// Trace Ray

		vkCmdTraceRaysKHR(
			renderCmdBuffer,
			&groupRegions[0],
			&groupRegions[1],
			&groupRegions[2],
			&groupRegions[3],
			swapChain.getExtent().width,
			swapChain.getExtent().height,
			1
		);

		// Tone Mapping

		renderTarget.transition_layout(
			_context, renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);
		

		uint32_t groupCountX = (swapChain.getExtent().width + 15) / 16;
		uint32_t groupCountY = (swapChain.getExtent().height + 15) / 16;

		vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);

		// Prepare for Blit
		ldrImage.transition_layout(
			_context, renderCmdBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		renderTarget.transition_layout(
			_context, renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);




		// Blit Pass


		swapChain.image_layout_transtion(
			imageIndex, renderCmdBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);


		_context.memAllocator().blit_image(
			renderCmdBuffer,
			ldrImage, ldrImage.layout, ldrImage.extent,
			swapChain.getImage(imageIndex), swapChain.getImageLayout(imageIndex), swapChain.getExtent(),
			VK_FILTER_NEAREST
		);



		// Prepare for Presentation

		swapChain.image_layout_transtion(
			imageIndex, renderCmdBuffer,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		// Resource Image Layout transition back

		ldrImage.transition_layout(
			_context, renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		
		// End and Submit

		renderCmdBuffer.end_and_submit(
			_context.gc_queue(),
			{ imageAvaiableSemaphore},
			{ VK_PIPELINE_STAGE_TRANSFER_BIT },
			{ renderFinishedSemaphore},
			inFlightFences[currentFrame]
		);

		result = swapChain.present(imageIndex, { renderFinishedSemaphore });

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) 
			throw std::runtime_error("failed to present swap chain image!");
		

		generalTimer.end();
		timerManager.print_real_fps("General");

		
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
}

void Renderer::offline_render(const std::string& name) {
	// Get Shader Groups Region
	groupRegions = _context.shaderManager().get_shader_group_regions();

	// Resources
	Image& renderTarget = hdrImages[0];

	VkDescriptorSet rtDynamicSet = *(descriptorSets.find("rtDynamicSet0")->second);
	VkDescriptorSet rtImageSet = *(descriptorSets.find("rtImageSet0")->second);
	VkDescriptorSet rtUniformSet = *(descriptorSets.find("rtUniformSet")->second);
	VkDescriptorSet rtSkyBoxSet = *(descriptorSets.find("rtSkyBoxSet")->second);

	pstd::vector<VkDescriptorSet> sets = { rtDynamicSet, rtImageSet,  rtUniformSet, rtSkyBoxSet };

	PushConstants pushConstants;

	Buffer readableBuffer = _context.memAllocator().create_buffer(
		renderTarget.extent.width * renderTarget.extent.height * 4 * sizeof(float),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	// Renderering Setting
	Scene::SceneDynamicInfo renderingSetting = scene.dynamicInfo;
	renderingSetting.iteration_depth = 16;
	renderingSetting.samples_per_pixel = 64;
	updateRenderingSetting(renderingSetting);



	// Timer
	Timer& offlineTimer = timerManager.register_timer("Offline");

	// CommandBuffer 
	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();


	// Start Rendering

	offlineTimer.start();
	
	cmdBuffer.begin();
	
	vkCmdBindPipeline(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		_context.rtPipeline()
	);
	
	vkCmdBindDescriptorSets(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		_context.rtPipeline().pipeline_layout(),
		0,
		static_cast<uint32_t>(sets.size()), sets.data(),
		0, nullptr
	);


	const uint32_t sample_batch = 1024;

	for (uint32_t current_batch = 0; current_batch < sample_batch; ++current_batch) {
		pushConstants.sample_batch = current_batch;

		vkCmdPushConstants(
			cmdBuffer,
			_context.rtPipeline().pipeline_layout(),
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			0, sizeof(pushConstants),
			&pushConstants
		);

		vkCmdTraceRaysKHR(
			cmdBuffer,
			&groupRegions[0],
			&groupRegions[1],
			&groupRegions[2],
			&groupRegions[3],
			renderTarget.extent.width,
			renderTarget.extent.height,
			1
		);

		/*if (current_batch < sample_batch - 1) {
			renderTarget.transition_layout(
				_context, cmdBuffer,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
			);
		}*/
			
	}


	renderTarget.transition_layout(
		_context, cmdBuffer,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);


	_context.memAllocator().copy_to_buffer(cmdBuffer, renderTarget, readableBuffer);

	renderTarget.transition_layout(
		_context, cmdBuffer,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
	);

	cmdBuffer.end_and_submit(_context.gc_queue(), true);


	offlineTimer.end();


	

	stbi_write_hdr(
		name.c_str(), 
		renderTarget.extent.width, renderTarget.extent.height, 4, 
		static_cast<float*>(readableBuffer.map_memory()));

	readableBuffer.unmap_memory();

}