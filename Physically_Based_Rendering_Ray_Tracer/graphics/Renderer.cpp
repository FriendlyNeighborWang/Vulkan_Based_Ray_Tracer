#include "Renderer.h"

Renderer::Renderer(Context& context, Window& window, SwapChain& swapChain) :_context(context), window(window), swapChain(swapChain){
	// Load Dynamic Function
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(context, "vkCmdTraceRaysKHR"));

	// Get Shader Groups Region
	groupRegions = context.shaderManager().get_shader_group_regions();

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

void Renderer::render() {
	// Resources Set
	Image& renderTarget = *(images.find("renderTarget")->second);
	Image& ldrImage = *(images.find("ldrImage")->second);

	VkDescriptorSet rayTracingSet = *(descriptorSets.find("rayTracingSet")->second);
	VkDescriptorSet computeSet = *(descriptorSets.find("computeSet")->second);

	PushConstants pushConstants;
	FormatTransPushConstants ftPushConstants;

	ComputePipeline& ftPipeline = *(computePipelines.find("ftPipeline")->second);

	// Rendering Info
	Timer& generalTimer = timerManager.register_timer("General");

	// Render
	while (!window.should_close()) {
		generalTimer.start();

		window.poll_events();

		inFlightFences[currentFrame].wait();

		uint32_t imageIndex;
		VkResult result = swapChain.acquire_next_image(swapchainImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Renderer::Failed to acquire swap chain image");

		inFlightFences[currentFrame].reset();



		cmdBuffers[currentFrame].begin();

		// Bind Pipeline & Descriptor Set
		vkCmdBindPipeline(cmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _context.rtPipeline());

		vkCmdBindDescriptorSets(
			cmdBuffers[currentFrame],
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			_context.rtPipeline().pipeline_layout(),
			0,
			1, &rayTracingSet,
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
			1, &computeSet,
			0, nullptr
		);

		ftPushConstants.exposure = 1.0f;

		vkCmdPushConstants(
			cmdBuffers[currentFrame],
			ftPipeline.pipeline_layout(),
			VK_SHADER_STAGE_COMPUTE_BIT,
			0, sizeof(ftPushConstants),
			&ftPushConstants
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

		// Format Transfer

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

		swapChain.present(imageIndex, submitSignalSemaphore);


		generalTimer.end();
		timerManager.print_real_fps("General");

		
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	
	

}