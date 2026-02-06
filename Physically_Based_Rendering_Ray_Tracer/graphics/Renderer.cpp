#include "Renderer.h"


#include "stb_image_write.h"

Renderer::Renderer(Context& context, Window& window, SwapChain& swapChain) :_context(context), window(window), swapChain(swapChain){
	// Load Dynamic Function
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(context, "vkCmdTraceRaysKHR"));

	// Get Shader Groups Region
	groupRegions = context.shaderManager().get_shader_group_regions();
}

void Renderer::register_image(std::string name, Image& image) {
	images.insert({ name, &image });
}

void Renderer::register_descriptor_set(std::string name, DescriptorSet& descriptorSet) {
	descriptorSets.insert({ name, &descriptorSet });
}

void Renderer::register_compute_pipeline(std::string name, ComputePipeline& pipeline) {
	computePipelines.insert({ name, &pipeline });
}

void Renderer::recreateSwapChain() {
	swapChain.recreate(window);
	
	// Image Recreate & Layout Transtion

	Image& renderTarget = *(images.find("renderTarget")->second);
	Image& ldrImage = *(images.find("ldrImage")->second);

	renderTarget = _context.memAllocator().create_image(
		swapChain.getExtent(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	ldrImage = _context.memAllocator().create_image(
		swapChain.getExtent(),
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	
	{
		CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
		cmdBuffer.begin();
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

		cmdBuffer.end_and_submit(_context.gc_queue(), true);
	}

	_context.descriptorManager().descriptor_write("RAY_TRACING_IMAGE_SET", BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, renderTarget);

	_context.descriptorManager().descriptor_write("COMPUTE_TONE_MAPPING_SET", BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, renderTarget);
	_context.descriptorManager().descriptor_write("COMPUTE_TONE_MAPPING_SET", BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, ldrImage);

	_context.descriptorManager().update_descriptor_set();
}

void Renderer::realtime_render() {
	// Resources Set
	Image& renderTarget = *(images.find("renderTarget")->second);
	Image& ldrImage = *(images.find("ldrImage")->second);

	VkDescriptorSet rtImageSet = *(descriptorSets.find("rtImageSet")->second);
	VkDescriptorSet rtUniformSet = *(descriptorSets.find("rtUniformSet")->second);
	VkDescriptorSet computeToneMappingSet = *(descriptorSets.find("computeToneMappingSet")->second);

	VkDescriptorSet render_sets[] = { rtImageSet, rtUniformSet };

	PushConstants pushConstants;
	ToneMappingPushConstants tmPushConstants;

	ComputePipeline& ftPipeline = *(computePipelines.find("tmPipeline")->second);

	// Timer
	Timer& generalTimer = timerManager.register_timer("General");

	// CommandBuffer
	while (_context.cmdPool().available_num() < MAX_FRAMES_IN_FLIGHT)
		_context.cmdPool().allocate_command_buffer();

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		cmdBuffers.push_back(std::move(_context.cmdPool().get_command_buffer()));
	}

	// Sync Object
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		inFlightFences.emplace_back(_context, true);
		swapchainImageAvailableSemaphores.emplace_back(_context);
	}

	for (uint32_t i = 0; i < swapChain.imageCount(); ++i) {
		renderFinishedSemaphores.emplace_back(_context);
	}

	// Render
	while (!window.should_close()) {
		window.poll_events();

		inFlightFences[currentFrame].wait();

		uint32_t imageIndex;
		VkResult result = swapChain.acquire_next_image(swapchainImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			continue;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Renderer::Failed to acquire swap chain image");

		inFlightFences[currentFrame].reset();

		// Start Timing
		generalTimer.start();

		cmdBuffers[currentFrame].begin();

		// Bind Pipeline & Descriptor Set
		vkCmdBindPipeline(cmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _context.rtPipeline());

		vkCmdBindDescriptorSets(
			cmdBuffers[currentFrame],
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			_context.rtPipeline().pipeline_layout(),
			0,
			2, render_sets,
			0, nullptr
		);


		pushConstants.sample_batch = 0;

		vkCmdPushConstants(
			cmdBuffers[currentFrame],
			_context.rtPipeline().pipeline_layout(),
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			0, sizeof(pushConstants),
			&pushConstants
		);


		vkCmdBindPipeline(cmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, ftPipeline);

		vkCmdBindDescriptorSets(
			cmdBuffers[currentFrame],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			ftPipeline.pipeline_layout(),
			0,
			1, &computeToneMappingSet, 
			0, nullptr
		);

		tmPushConstants.exposure = 0.5f;

		vkCmdPushConstants(
			cmdBuffers[currentFrame],
			ftPipeline.pipeline_layout(),
			VK_SHADER_STAGE_COMPUTE_BIT,
			0, sizeof(tmPushConstants),
			&tmPushConstants
		);

		// Trace Ray

		vkCmdTraceRaysKHR(
			cmdBuffers[currentFrame],
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
			_context, cmdBuffers[currentFrame],
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);


		

		uint32_t groupCountX = (swapChain.getExtent().width + 15) / 16;
		uint32_t groupCountY = (swapChain.getExtent().height + 15) / 16;

		vkCmdDispatch(cmdBuffers[currentFrame], groupCountX, groupCountY, 1);

		// Blit

		ldrImage.transition_layout(
			_context, cmdBuffers[currentFrame],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		swapChain.image_layout_transtion(
			imageIndex, cmdBuffers[currentFrame],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);


		_context.memAllocator().blit_image(
			cmdBuffers[currentFrame],
			ldrImage, ldrImage.layout, ldrImage.extent,
			swapChain.getImage(imageIndex), swapChain.getImageLayout(imageIndex), swapChain.getExtent(),
			VK_FILTER_NEAREST
		);



		// Prepare for Presentation

		swapChain.image_layout_transtion(
			imageIndex, cmdBuffers[currentFrame],
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		// Resource Image Layout transition back

		renderTarget.transition_layout(
			_context, cmdBuffers[currentFrame],
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);

		ldrImage.transition_layout(
			_context, cmdBuffers[currentFrame],
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		
		// End and Submit
		pstd::vector<VkSemaphore> submitWaitSemaphore{ swapchainImageAvailableSemaphores[currentFrame] };
		pstd::vector<VkSemaphore> submitSignalSemaphore{ renderFinishedSemaphores[imageIndex] };
		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_TRANSFER_BIT;

		cmdBuffers[currentFrame].end_and_submit(
			_context.gc_queue(),
			submitWaitSemaphore,
			waitStages,
			submitSignalSemaphore,
			inFlightFences[currentFrame]
		);

		result = swapChain.present(imageIndex, submitSignalSemaphore);

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
	// Resources
	Image& renderTarget = *(images.find("renderTarget")->second);

	VkDescriptorSet rtImageSet = *(descriptorSets.find("rtImageSet")->second);
	VkDescriptorSet rtUniformSet = *(descriptorSets.find("rtUniformSet")->second);
	VkDescriptorSet sets[2] = { rtImageSet, rtUniformSet };

	PushConstants pushConstants;

	Buffer readableBuffer = _context.memAllocator().create_buffer(
		renderTarget.extent.width * renderTarget.extent.height * 4 * sizeof(float),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

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
		2, sets,
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