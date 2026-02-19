#ifndef PBRT_GRAPHICS_DRAW_FRAME
#define PBRT_GRAPHICS_DRAW_FRAME

#include "base/base.h"
#include "util/Timer.h"
#include "Window.h"

#include <unordered_map>


class Renderer {
public:
	Renderer(Context& context, Window& window, SwapChain& swapChain, Scene& scene);

	void register_image(std::string name, Image& image);

	void register_descriptor_set(std::string name, DescriptorSet& descriptorSet);

	void register_compute_pipeline(std::string name, ComputePipeline& pipeline);

	void realtime_render();
	void offline_render(const std::string& name);

private:

	void recreateSwapChain();

	void updateDynamicSceneInfo();

	// Rendering Info
	pstd::vector<VkStridedDeviceAddressRegionKHR> groupRegions;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	// Sync Object
	pstd::vector<Semaphore> swapchainImageAvailableSemaphores;
	pstd::vector<Semaphore> renderFinishedSemaphores;
	pstd::vector<Fence> inFlightFences;


	// Resource
	pstd::vector<CommandBuffer> cmdBuffers;
	std::unordered_map<std::string, Image*> images;
	std::unordered_map<std::string, DescriptorSet*> descriptorSets;
	std::unordered_map<std::string, ComputePipeline*> computePipelines;

	// Reference
	Window& window;
	SwapChain& swapChain;
	Scene& scene;
	Context& _context;

	// Else
	TimerManager timerManager;

	uint32_t currentFrame = 0;
};



#endif // !PBRT_GRAPHICS_DRAW_FRAME

