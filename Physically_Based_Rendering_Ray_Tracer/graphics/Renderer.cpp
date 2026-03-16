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

#include "FrameContext.h"
#include "ResourceManager.h"

#include "stb_image_write.h"

#include <cstdlib>

Renderer::Renderer(Context& context, Window& window, SwapChain& swapChain, Scene& scene) :_context(context), window(window), swapChain(swapChain), scene(scene), inputManager(window, scene.dynamicInfo){
	// Load Dynamic Function
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(context, "vkCmdTraceRaysKHR"));
}
Renderer::~Renderer() = default;

void Renderer::prepare_frame_context() {
	while (_context.cmdPool().available_num() < MAX_FRAMES_IN_FLIGHT + 3)
		_context.cmdPool().allocate_command_buffer();

	for (uint32_t frame_idx = 0; frame_idx < MAX_FRAMES_IN_FLIGHT; ++frame_idx) {
		FrameContext frameContext;
		frameContext.cmdBuffer = _context.cmdPool().get_command_buffer();
		for (uint32_t pipeline_id : _context.pipelineManager().all_ids()) {
			std::string name = _context.pipelineManager().get_name(pipeline_id);
			pstd::vector descriptor_sets = _context.resourceManager().get_descriptor_sets(pipeline_id, frame_idx);
			frameContext.descripotrSets.insert({ name, std::vector<VkDescriptorSet>(descriptor_sets.begin(), descriptor_sets.end())});
		}

		// TODO:: Hard Coding now, will be changed by Render Graph
		frameContext.images.insert({ "HDR_IMAGE", _context.resourceManager().get_resource<Image>("HDR_IMAGE" + std::to_string(frame_idx)) });
		frameContext.images.insert({ "LDR_IMAGE" , _context.resourceManager().get_resource<Image>("LDR_IMAGE" + std::to_string(frame_idx)) });

		frameContext.buffers.insert({ "DYNAMIC_INFO", _context.resourceManager().get_resource<Buffer>("DYNAMIC_INFO" + std::to_string(frame_idx)) });

		frames.push_back(std::move(frameContext));
	}
	
}

void Renderer::recreateSwapChain() {
	swapChain.recreate(window);
	
	// Image Recreate & Layout Transtion

	_context.resourceManager().rebuild_window_size_related_resources(swapChain.getExtent());

	
}

void Renderer::updateDynamicSceneInfo(Timer& timer, FrameContext& fc) {
	// Update Data
	inputManager.update(timer.get_delta());

	auto& dynamic_info = scene.dynamicInfo;
	
	// Write into Uniform Buffer
	auto& dynamic_info_buffer = fc.buffers.find("DYNAMIC_INFO")->second;

	dynamic_info_buffer->write_buffer(&dynamic_info, sizeof(dynamic_info));

}

void Renderer::updateRenderingSetting(Scene::SceneDynamicInfo dynamicInfo) {
	for (auto& fc : frames) {
		auto& dynamic_info_buffer = fc.buffers.find("DYNAMIC_INFO")->second;
		dynamic_info_buffer->write_buffer(&dynamicInfo, sizeof(dynamicInfo));
	}
}

void Renderer::realtime_render() {
	// Get Pipeline
	RTPipeline& rtPipeline = static_cast<RTPipeline&>(_context.pipelineManager().get("RAY_TRACING"));
	ComputePipeline& tonemappingPipeline = static_cast<ComputePipeline&>(_context.pipelineManager().get("TONE_MAPPING"));

	// Get Shader Groups Region
	auto& groupRegions = static_cast<RTPipeline&>(_context.pipelineManager().get("RAY_TRACING")).get_shader_group_regions();

	// Push Constant
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
	renderingSeting.samples_per_pixel = 8;
	updateRenderingSetting(renderingSeting);

	// Render
	while (!window.should_close()) {
		window.poll_events();

		// Snap Shot
		if (inputManager.pressed_trigger({ GLFW_KEY_LEFT_SHIFT, GLFW_KEY_S })) {
			inputManager.release_cursor();
			vkDeviceWaitIdle(_context);
			offline_render("snapshot.hdr");
			break;
		}


		inFlightFences[frame_idx].wait();
		

		// Resource Setting & Acquire Next Image
		FrameContext& fc = frames[frame_idx];
		CommandBuffer& renderCmdBuffer = fc.cmdBuffer;
		Image& renderTarget = *fc.images.at("HDR_IMAGE");
		Image& ldrImage = *fc.images.at("LDR_IMAGE");
		Semaphore& imageAvaiableSemaphore = imageAvailableSemaphores[frame_idx];

		uint32_t imageIndex;
		VkResult result = swapChain.acquire_next_image(imageAvaiableSemaphore, &imageIndex);

		Semaphore& renderFinishedSemaphore = renderFinishedSemaphores[imageIndex];

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			continue;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Renderer::Failed to acquire swap chain image");

		inFlightFences[frame_idx].reset();

		// Start Timing
		generalTimer.start();


		// Render Pass

		renderCmdBuffer.begin();

		// Bind Pipeline & Descriptor Set
		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);

		updateDynamicSceneInfo(deltaTimer, fc);
		std::vector<VkDescriptorSet>& render_sets = fc.descripotrSets.at("RAY_TRACING");

		vkCmdBindDescriptorSets(
			renderCmdBuffer,
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			rtPipeline.pipeline_layout(),
			0,
			render_sets.size(), render_sets.data(),
			0, nullptr
		);
		

		// pushConstants.current_frame = currentFrame++;
		pushConstants.sample_batch = 0;

		vkCmdPushConstants(
			renderCmdBuffer,
			rtPipeline.pipeline_layout(),
			VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			0, sizeof(pushConstants),
			&pushConstants
		);


		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, tonemappingPipeline);
		
		std::vector<VkDescriptorSet>& tone_mapping_sets = fc.descripotrSets.at("TONE_MAPPING");

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

		// Image Layout Transition
		renderTarget.transition_layout(
			_context, renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
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
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		ldrImage.transition_layout(
			_context, renderCmdBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);
		

		uint32_t groupCountX = (swapChain.getExtent().width + 15) / 16;
		uint32_t groupCountY = (swapChain.getExtent().height + 15) / 16;

		vkCmdDispatch(renderCmdBuffer, groupCountX, groupCountY, 1);





		// Blit Pass

		// Prepare for Blit
		ldrImage.transition_layout(
			_context, renderCmdBuffer,
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



		// Prepare for Presentation

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
			{ imageAvaiableSemaphore},
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

		
		frame_idx = (frame_idx + 1) % MAX_FRAMES_IN_FLIGHT;
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
	ComputePipeline& tonemappingPipeline = static_cast<ComputePipeline&>(_context.pipelineManager().get("TONE_MAPPING"));

	// Get Shader Groups Region
	auto& groupRegions = static_cast<RTPipeline&>(_context.pipelineManager().get("RAY_TRACING")).get_shader_group_regions();

	// Resources
	FrameContext& fc = frames[frame_idx];
	CommandBuffer& cmdBuffer = fc.cmdBuffer;
	Image& renderTarget = *fc.images.at("HDR_IMAGE");

	std::vector<VkDescriptorSet>& render_sets = fc.descripotrSets.at("RAY_TRACING");
	std::vector<VkDescriptorSet>& tone_mapping_sets = fc.descripotrSets.at("TONE_MAPPING");

	PushConstants pushConstants;

	Buffer readableBuffer = _context.memAllocator().create_buffer(
		renderTarget.extent.width * renderTarget.extent.height * 4 * sizeof(float),
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
		_context, cmdBuffer,
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
			&groupRegions[0],
			&groupRegions[1],
			&groupRegions[2],
			&groupRegions[3],
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
		_context, cmdBuffer,
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

