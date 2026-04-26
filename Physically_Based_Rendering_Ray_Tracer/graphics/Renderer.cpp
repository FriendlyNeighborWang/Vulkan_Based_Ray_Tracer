#include "Renderer.h"


#include "Scene.h"
#include "vk_layer/Image.h"
#include "vk_layer/Context.h"
#include "vk_layer/Pipeline.h"
#include "vk_layer/SwapChain.h"
#include "vk_layer/SyncObject.h"
#include "vk_layer/CommandPool.h"
#include "vk_layer/PipelineManager.h"
#include "vk_layer/DescriptorManager.h"
#include "vk_layer/VkMemoryAllocator.h"

#include "ResourceManager.h"

#include "stb_image_write.h"


Renderer::Renderer(Context& context, Window& window, SwapChain& swapChain, Scene& scene) :_context(context), window(window), swapChain(swapChain), scene(scene), inputManager(window, scene.dynamicInfo){
	// Load Dynamic Function
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(context, "vkCmdTraceRaysKHR"));
}
Renderer::~Renderer() = default;


void Renderer::recreateSwapChain() {
	swapChain.recreate(window);
	
	// Image Recreate & Layout Transtion

	_context.resourceManager().rebuild_window_size_related_resources(swapChain.getExtent());

	
}

void Renderer::updateDynamicSceneInfo(Timer& timer, Buffer& dynamicInfoBuffer) {
	// Update Data
	inputManager.update(timer.get_delta());

	auto& dynamic_info = scene.dynamicInfo;
	
	// Update ViewProj
	dynamic_info.prevViewProj = dynamic_info.viewProj;

	glm::mat4 view = Scene::look_at(
		dynamic_info.camera.position,
		dynamic_info.camera.position + dynamic_info.camera.direction,
		dynamic_info.camera.up
	);

	glm::mat4 proj = Scene::perspective(
		dynamic_info.camera.yfov,
		float(swapChain.getExtent().width) / float(swapChain.getExtent().height),
		dynamic_info.camera.znear, dynamic_info.camera.zfar
	);
	proj[1][1] *= -1;
	dynamic_info.viewProj = proj * view;

	// Write into Uniform Buffer

	dynamicInfoBuffer.write_buffer(&dynamic_info, sizeof(dynamic_info));

}

void Renderer::updateRenderingSetting(Scene::SceneDynamicInfo dynamicInfo) {
	for (uint32_t i = 0; i<MAX_FRAMES_IN_FLIGHT; ++i)
	{
		auto* dynamic_info_buffer = _context.resourceManager().get_resource<Buffer>("DYNAMIC_INFO" + std::to_string(i));
		dynamic_info_buffer->write_buffer(&dynamicInfo, sizeof(dynamicInfo));
	}
	
}

void Renderer::realtime_render() {
	// Get Pipeline
	Pipeline& gBufferPipeline = _context.pipelineManager().get("G_BUFFER");
	Pipeline& initializeReservoirPipeline = _context.pipelineManager().get("INITIALIZE_RESERVOIR");
	Pipeline& temporalReusePipeline = _context.pipelineManager().get("TEMPORAL_REUSE");
	Pipeline& spatialReusePipeline = _context.pipelineManager().get("SPATIAL_REUSE");
	Pipeline& rtPipeline = _context.pipelineManager().get("RAY_TRACING");
	Pipeline& tonemappingPipeline = _context.pipelineManager().get("TONE_MAPPING");

	// Get Shader Groups Region
	auto& gBufferGroupRegions = static_cast<RTPipeline&>(gBufferPipeline).get_shader_group_regions();
	auto& groupRegions = static_cast<RTPipeline&>(rtPipeline).get_shader_group_regions();

	// Push Constant
	ReservoirPushConstants reservoirPushConstants;
	PushConstants pushConstants;
	ToneMappingPushConstants tmPushConstants;


	// Timer
	Timer& generalTimer = timerManager.register_timer("General");
	Timer& deltaTimer = timerManager.register_timer("Delta");

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
	renderingSeting.iteration_depth = 3;
	renderingSeting.samples_per_pixel = 16;

	auto& cam = renderingSeting.camera;
	glm::mat4 view = Scene::look_at(cam.position, cam.position + cam.direction, cam.up);
	float aspect = static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height);
	glm::mat4 proj = Scene::perspective(cam.yfov, aspect, cam.znear, cam.zfar);
	proj[1][1] *= -1;
	renderingSeting.viewProj = proj * view;

	updateRenderingSetting(renderingSeting);

	bool if_restir = false;

	// Render
	while (!window.should_close()) {
		window.poll_events();

		// Snap Shot
		if (inputManager.pressed_trigger({ GLFW_KEY_CAPS_LOCK, GLFW_KEY_S })) {
			inputManager.release_cursor();
			vkDeviceWaitIdle(_context);
			offline_render("snapshot.hdr");
			break;
		}

		// Switch Render Pipeline
		if (inputManager.pressed_trigger({GLFW_KEY_RIGHT_ALT}, true))
		{
			if_restir = !if_restir;
		}


		inFlightFences[frame_idx].wait();


		// Acquire Next Image
		Semaphore& imageAvailableSemaphore = imageAvailableSemaphores[frame_idx];
		uint32_t imageIndex;
		VkResult result = swapChain.acquire_next_image(imageAvailableSemaphore, &imageIndex);

		Semaphore& renderFinishedSemaphore = renderFinishedSemaphores[imageIndex];

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			continue;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Renderer::Failed to acquire swap chain image");

		inFlightFences[frame_idx].reset();

		// Resource Setting 
		CommandBuffer renderCmdBuffer = _context.cmdPool().get_command_buffer();
		Image& renderTarget = *_context.resourceManager().get_resource<Image>("HDR_IMAGE" + std::to_string(frame_idx));
		Image& ldrImage = *_context.resourceManager().get_resource<Image>("LDR_IMAGE" + std::to_string(frame_idx));

		Buffer& dynamicInfoBuffer = *_context.resourceManager().get_resource<Buffer>("DYNAMIC_INFO" + std::to_string(frame_idx));
		Buffer& gBuffer_current = *_context.resourceManager().get_resource<Buffer>("G_BUFFER" + std::to_string(temporal_idx));
		Buffer& gBuffer_prev = *_context.resourceManager().get_resource<Buffer>("G_BUFFER" + std::to_string((temporal_idx + 1) % 2));
		Buffer& reservoirBuffer_current = *_context.resourceManager().get_resource<Buffer>("RESERVOIR_BUFFER" + std::to_string(temporal_idx));
		Buffer& reservoirBuffer_prev = *_context.resourceManager().get_resource<Buffer>("RESERVOIR_BUFFER" + std::to_string((temporal_idx + 1) % 2));

		pstd::vector<VkDescriptorSet> gBuffer_sets = _context.resourceManager().get_descriptor_sets("G_BUFFER", frame_idx, temporal_idx);
		pstd::vector<VkDescriptorSet> init_res_sets = _context.resourceManager().get_descriptor_sets("INITIALIZE_RESERVOIR", frame_idx, temporal_idx);
		pstd::vector<VkDescriptorSet> temporal_reuse_sets = _context.resourceManager().get_descriptor_sets("TEMPORAL_REUSE", frame_idx, temporal_idx);
		pstd::vector<VkDescriptorSet> spatial_reuse_sets = _context.resourceManager().get_descriptor_sets("SPATIAL_REUSE", frame_idx, temporal_idx);
		pstd::vector<VkDescriptorSet> render_sets = _context.resourceManager().get_descriptor_sets("RAY_TRACING", frame_idx, temporal_idx);
		pstd::vector<VkDescriptorSet> tone_mapping_sets = _context.resourceManager().get_descriptor_sets("TONE_MAPPING", frame_idx, temporal_idx);


		// Start Timing
		generalTimer.start();

		renderCmdBuffer.begin();

		updateDynamicSceneInfo(deltaTimer, dynamicInfoBuffer);

		if (if_restir)
		{
			// ===================== Pass 1: G-Buffer (RT) =====================
			gBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, gBufferPipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				gBufferPipeline.pipeline_layout(),
				0,
				gBuffer_sets.size(), gBuffer_sets.data(),
				0, nullptr
			);

			vkCmdTraceRaysKHR(
				renderCmdBuffer,
				gBufferGroupRegions.rayGenRegion.data(),
				&gBufferGroupRegions.missRegion,
				&gBufferGroupRegions.hitRegion,
				&gBufferGroupRegions.callableRegion,
				swapChain.getExtent().width,
				swapChain.getExtent().height,
				1
			);

			// ===================== Pass 2: Init Reservoir (Compute) =====================
			gBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			reservoirBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, initializeReservoirPipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				initializeReservoirPipeline.pipeline_layout(),
				0,
				init_res_sets.size(), init_res_sets.data(),
				0, nullptr
			);

			reservoirPushConstants.current_frame = currentFrame;
			reservoirPushConstants.screen_width = swapChain.getExtent().width;
			reservoirPushConstants.screen_height = swapChain.getExtent().height;

			vkCmdPushConstants(
				renderCmdBuffer,
				initializeReservoirPipeline.pipeline_layout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(reservoirPushConstants),
				&reservoirPushConstants
			);

			uint32_t groupCountX = (swapChain.getExtent().width + 15) / 16;
			uint32_t groupCountY = (swapChain.getExtent().height + 15) / 16;

			vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);

			// ===================== Pass 3: Temporal Reuse (Compute) =====================
			gBuffer_prev.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			reservoirBuffer_prev.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			reservoirBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, temporalReusePipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				temporalReusePipeline.pipeline_layout(),
				0,
				temporal_reuse_sets.size(), temporal_reuse_sets.data(),
				0, nullptr
			);

			vkCmdPushConstants(
				renderCmdBuffer,
				temporalReusePipeline.pipeline_layout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(reservoirPushConstants),
				&reservoirPushConstants
			);

			vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);

			// ===================== Pass 4: Visibility Reuse (RT) =====================
			gBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
			reservoirBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				rtPipeline.pipeline_layout(),
				0,
				render_sets.size(), render_sets.data(),
				0, nullptr
			);

			vkCmdTraceRaysKHR(
				renderCmdBuffer,
				&groupRegions.rayGenRegion[2],
				&groupRegions.missRegion,
				&groupRegions.hitRegion,
				&groupRegions.callableRegion,
				swapChain.getExtent().width,
				swapChain.getExtent().height,
				1
			);

			// ===================== Pass 5: Spatial Reuse (Compute) =====================
			gBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			reservoirBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, spatialReusePipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				spatialReusePipeline.pipeline_layout(),
				0,
				spatial_reuse_sets.size(), spatial_reuse_sets.data(),
				0, nullptr
			);

			vkCmdPushConstants(
				renderCmdBuffer,
				spatialReusePipeline.pipeline_layout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(reservoirPushConstants),
				&reservoirPushConstants
			);

			vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);

			reservoirBuffer_prev.barrier(renderCmdBuffer, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			reservoirBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			_context.memAllocator().copy_to_buffer(renderCmdBuffer, reservoirBuffer_prev, reservoirBuffer_current);

			// ===================== Pass 6: ReSTIR Shading (RT) =====================
			gBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
			reservoirBuffer_current.barrier(renderCmdBuffer, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

			renderTarget.transition_layout(
				renderCmdBuffer,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
			);


			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				rtPipeline.pipeline_layout(),
				0,
				render_sets.size(), render_sets.data(),
				0, nullptr
			);


			pushConstants.current_frame = currentFrame;
			pushConstants.sample_batch = 0;

			vkCmdPushConstants(
				renderCmdBuffer,
				rtPipeline.pipeline_layout(),
				VK_SHADER_STAGE_RAYGEN_BIT_KHR,
				0, sizeof(pushConstants),
				&pushConstants
			);

			vkCmdTraceRaysKHR(
				renderCmdBuffer,
				&groupRegions.rayGenRegion[1],
				&groupRegions.missRegion,
				&groupRegions.hitRegion,
				&groupRegions.callableRegion,
				swapChain.getExtent().width,
				swapChain.getExtent().height,
				1
			);

		}else
		{
			// ===================== Pass 1: Ray Tracing =====================
			renderTarget.transition_layout(
				renderCmdBuffer,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
			);

			vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);

			vkCmdBindDescriptorSets(
				renderCmdBuffer,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				rtPipeline.pipeline_layout(),
				0,
				render_sets.size(), render_sets.data(),
				0, nullptr
			);

			pushConstants.current_frame = currentFrame;
			pushConstants.sample_batch = 0;

			vkCmdPushConstants(
				renderCmdBuffer,
				rtPipeline.pipeline_layout(),
				VK_SHADER_STAGE_RAYGEN_BIT_KHR,
				0, sizeof(pushConstants),
				&pushConstants
			);

			vkCmdTraceRaysKHR(
				renderCmdBuffer,
				&groupRegions.rayGenRegion[0],
				&groupRegions.missRegion,
				&groupRegions.hitRegion,
				&groupRegions.callableRegion,
				swapChain.getExtent().width,
				swapChain.getExtent().height,
				1
			);

			
		}

		// ===================== Pass: Tone Mapping (Compute) =====================
		renderTarget.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		ldrImage.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, tonemappingPipeline);

		vkCmdBindDescriptorSets(
			renderCmdBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			tonemappingPipeline.pipeline_layout(),
			0,
			tone_mapping_sets.size(), tone_mapping_sets.data(),
			0, nullptr
		);

		tmPushConstants.exposure = 0.5f;

		vkCmdPushConstants(
			renderCmdBuffer,
			tonemappingPipeline.pipeline_layout(),
			VK_SHADER_STAGE_COMPUTE_BIT,
			0, sizeof(tmPushConstants),
			&tmPushConstants
		);

		uint32_t groupCountX = (swapChain.getExtent().width + 15) / 16;
		uint32_t groupCountY = (swapChain.getExtent().height + 15) / 16;

		vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);

		// ===================== Pass: Blit =====================
		ldrImage.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

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



		// ===================== Presentation =====================

		swapChain.image_layout_transtion(
			imageIndex, renderCmdBuffer,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		
		// End and Submit

		renderCmdBuffer.end_and_submit(
			_context.gc_queue(),
			{ imageAvailableSemaphore},
			{ VK_PIPELINE_STAGE_TRANSFER_BIT },
			{ renderFinishedSemaphore},
			inFlightFences[frame_idx]
		);

		result = swapChain.present(imageIndex, { renderFinishedSemaphore });

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) 
			throw std::runtime_error("failed to present swap chain image!");
		

		generalTimer.end();
		timerManager.print_real_fps("General");

		
		currentFrame++;
		frame_idx = currentFrame % MAX_FRAMES_IN_FLIGHT;
		temporal_idx = currentFrame % 2;
	}
}

void Renderer::offline_render(const std::string& name) {
	// Input Rendering Setting
	system("cls");

	auto readUnit = [](const std::string& prompt, uint32_t defaultValue)-> uint32_t {
		std::cout << prompt << " [default: " << defaultValue << "]: ";
		std::string line;
		std::getline(std::cin, line);
		if (line.empty()) return defaultValue;
		try { return static_cast<uint32_t>(std::stoul(line)); }
		catch (...) { return defaultValue; }
		};

	uint32_t iteration_depth = readUnit("Iteration Depth", scene.dynamicInfo.iteration_depth);
	uint32_t samples_per_pixel = readUnit("Samples Per Pixel", scene.dynamicInfo.samples_per_pixel);
	uint32_t sample_batch = readUnit("Sample Batch", 512);
	uint32_t image_width = readUnit("Resolution Width", 3840);
	uint32_t image_height = readUnit("Resolution Height", 2160);



	// Renderering Setting
	Scene::SceneDynamicInfo renderingSetting = scene.dynamicInfo;
	renderingSetting.iteration_depth = iteration_depth;
	renderingSetting.samples_per_pixel = samples_per_pixel;
	updateRenderingSetting(renderingSetting);

	// Image Rebuild
	_context.resourceManager().rebuild_window_size_related_resources({ image_width, image_height });

	// Get Pipeline
	RTPipeline& rtPipeline = static_cast<RTPipeline&>(_context.pipelineManager().get("RAY_TRACING"));

	// Get Shader Groups Region
	auto& groupRegions = rtPipeline.get_shader_group_regions();

	// Resources
	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	Image& renderTarget = *_context.resourceManager().get_resource<Image>("HDR_IMAGE" + std::to_string(frame_idx));

	pstd::vector<VkDescriptorSet> render_sets = _context.resourceManager().get_descriptor_sets("RAY_TRACING", 0, 0);

	PushConstants pushConstants;

	Buffer readableBuffer = _context.memAllocator().create_buffer(
		renderTarget.extent.width * renderTarget.extent.height * 4 * sizeof(float),
		0,
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	

	// Timer
	Timer& offlineTimer = timerManager.register_timer("Offline");


	// Start Rendering

	offlineTimer.start();
	
	cmdBuffer.begin();
	
	vkCmdBindPipeline(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		rtPipeline
	);
	
	vkCmdBindDescriptorSets(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		rtPipeline.pipeline_layout(),
		0,
		render_sets.size(), render_sets.data(),
		0, nullptr
	);

	// Layout Transition
	renderTarget.transition_layout(
		cmdBuffer,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
	);


	

	for (uint32_t current_batch = 0; current_batch < sample_batch; ++current_batch) {
		pushConstants.sample_batch = current_batch;

		vkCmdPushConstants(
			cmdBuffer,
			rtPipeline.pipeline_layout(),
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			0, sizeof(pushConstants),
			&pushConstants
		);

		vkCmdTraceRaysKHR(
			cmdBuffer,
			&groupRegions.rayGenRegion[0],
			&groupRegions.missRegion,
			&groupRegions.hitRegion,
			&groupRegions.callableRegion,
			renderTarget.extent.width,
			renderTarget.extent.height,
			1
		);

		// if (current_batch < sample_batch - 1) {
		// 	renderTarget.transition_layout(
		//		_context, cmdBuffer,
		//		VK_IMAGE_LAYOUT_GENERAL,
		//		VK_ACCESS_SHADER_WRITE_BIT,
		//		VK_ACCESS_SHADER_READ_BIT,
		//		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		//		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		//	);
		// }
			
	}

	// Layout Transition

	renderTarget.transition_layout(
		cmdBuffer,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);


	_context.memAllocator().copy_to_buffer(cmdBuffer, renderTarget, readableBuffer);

	cmdBuffer.end_and_submit(_context.gc_queue(), true);


	offlineTimer.end();


	

	stbi_write_hdr(
		name.c_str(), 
		renderTarget.extent.width, renderTarget.extent.height, 4, 
		static_cast<float*>(readableBuffer.map_memory()));

	readableBuffer.unmap_memory();

}

void Renderer::original_render()
{
	// Get Pipeline
	Pipeline& rtPipeline = _context.pipelineManager().get("RAY_TRACING");
	Pipeline& tonemappingPipeline = _context.pipelineManager().get("TONE_MAPPING");

	// Get Shader Groups Region
	auto& groupRegions = static_cast<RTPipeline&>(rtPipeline).get_shader_group_regions();

	PushConstants pushConstants;
	ToneMappingPushConstants tmPushConstants;

	Timer& generalTimer = timerManager.register_timer("General");
	Timer& deltaTimer = timerManager.register_timer("Delta");
	for (uint32_t i =0;i<MAX_FRAMES_IN_FLIGHT; ++i)
	{
		inFlightFences.emplace_back(_context, true);
		imageAvailableSemaphores.emplace_back(_context);
	}

	for (uint32_t i = 0; i<swapChain.imageCount(); ++i)
	{
		renderFinishedSemaphores.emplace_back(_context);
	}

	auto& renderingSeting = scene.dynamicInfo;
	renderingSeting.iteration_depth = 3;
	renderingSeting.samples_per_pixel = 4;

	auto& cam = renderingSeting.camera;
	glm::mat4 view = Scene::look_at(cam.position, cam.position + cam.direction, cam.up);
	float aspect = static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height);
	glm::mat4 proj = Scene::perspective(cam.yfov, aspect, cam.znear, cam.zfar);
	proj[1][1] *= -1;
	renderingSeting.viewProj = proj * view;

	updateRenderingSetting(renderingSeting);

	while (!window.should_close())
	{
		window.poll_events();

		// Snap Shot
		if (inputManager.pressed_trigger({ GLFW_KEY_CAPS_LOCK, GLFW_KEY_S })) {
			inputManager.release_cursor();
			vkDeviceWaitIdle(_context);
			offline_render("snapshot.hdr");
			break;
		}

		inFlightFences[frame_idx].wait();

		Semaphore& imageAvailableSemaphore = imageAvailableSemaphores[frame_idx];
		uint32_t imageIndex;
		VkResult result = swapChain.acquire_next_image(imageAvailableSemaphore, &imageIndex);

		Semaphore& renderFinishedSemaphore = renderFinishedSemaphores[imageIndex];
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			continue;
		}else if (result !=VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Renderer::Failed to acquire swap chain image");
		}

		inFlightFences[frame_idx].reset();

		CommandBuffer renderCmdBuffer = _context.cmdPool().get_command_buffer();
		Image& renderTarget = *_context.resourceManager().get_resource<Image>("HDR_IMAGE" + std::to_string(frame_idx));
		Image& ldrImage = *_context.resourceManager().get_resource<Image>("LDR_IMAGE" + std::to_string(frame_idx));
		Buffer& dynamicInfoBuffer = *_context.resourceManager().get_resource<Buffer>("DYNAMIC_INFO" + std::to_string(frame_idx));

		pstd::vector<VkDescriptorSet> render_sets = _context.resourceManager().get_descriptor_sets("RAY_TRACING", frame_idx, temporal_idx);
		pstd::vector<VkDescriptorSet> tone_mapping_sets = _context.resourceManager().get_descriptor_sets("TONE_MAPPING", frame_idx, temporal_idx);

		generalTimer.start();

		renderCmdBuffer.begin();

		updateDynamicSceneInfo(deltaTimer, dynamicInfoBuffer);

		// ===================== Pass 1: Ray Tracing =====================
		renderTarget.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);

		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);

		vkCmdBindDescriptorSets(
			renderCmdBuffer,
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			rtPipeline.pipeline_layout(),
			0,
			render_sets.size(), render_sets.data(),
			0, nullptr
		);

		// pushConstants.current_frame = currentFrame;
		pushConstants.sample_batch = 0;

		vkCmdPushConstants(
			renderCmdBuffer,
			rtPipeline.pipeline_layout(),
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			0, sizeof(pushConstants),
			&pushConstants
		);

		vkCmdTraceRaysKHR(
			renderCmdBuffer,
			&groupRegions.rayGenRegion[0],
			&groupRegions.missRegion,
			&groupRegions.hitRegion,
			&groupRegions.callableRegion,
			swapChain.getExtent().width,
			swapChain.getExtent().height,
			1
		);

		// ===================== Pass 2: Tone Mapping (Compute) =====================
		renderTarget.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		ldrImage.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, tonemappingPipeline);

		vkCmdBindDescriptorSets(
			renderCmdBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			tonemappingPipeline.pipeline_layout(),
			0,
			tone_mapping_sets.size(), tone_mapping_sets.data(),
			0, nullptr
		);

		tmPushConstants.exposure = 0.5f;

		vkCmdPushConstants(
			renderCmdBuffer,
			tonemappingPipeline.pipeline_layout(),
			VK_SHADER_STAGE_COMPUTE_BIT,
			0, sizeof(tmPushConstants),
			&tmPushConstants
		);

		uint32_t groupCountX = (swapChain.getExtent().width + 15) / 16;
		uint32_t groupCountY = (swapChain.getExtent().height + 15) / 16;

		vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);

		// ===================== Pass 3: Blit =====================
		ldrImage.transition_layout(
			renderCmdBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

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

		// ===================== Presentation =====================
		swapChain.image_layout_transtion(
			imageIndex, renderCmdBuffer,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		// End and Submit
		renderCmdBuffer.end_and_submit(
			_context.gc_queue(),
			{ imageAvailableSemaphore },
			{ VK_PIPELINE_STAGE_TRANSFER_BIT },
			{ renderFinishedSemaphore },
			inFlightFences[frame_idx]
		);

		result = swapChain.present(imageIndex, { renderFinishedSemaphore });

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("failed to present swap chain image!");

		generalTimer.end();
		timerManager.print_real_fps("General");

		currentFrame++;
		frame_idx = currentFrame % MAX_FRAMES_IN_FLIGHT;
		temporal_idx = currentFrame % 2;


	}

}