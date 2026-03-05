#ifndef PBRT_GRAPHICS_RENDERER
#define PBRT_GRAPHICS_RENDERER

#include "base/base.h"
#include "util/Timer.h"
#include "CameraController.h"
#include "Window.h"

#include <unordered_map>


class Renderer {
public:
	Renderer(Context& context, Window& window, SwapChain& swapChain, Scene& scene);

	void create_uniform_buffer();
	const pstd::vector<Buffer>& get_uniform_buffers() const { return uniformBuffers; }

	void create_images();
	const pstd::vector<Image>& get_ldrImages() const { return ldrImages; }
	const pstd::vector<Image>& get_hdrImages() const { return hdrImages; }

	void register_descriptor_set(std::string name, DescriptorSet& descriptorSet);

	void register_compute_pipeline(std::string name, ComputePipeline& pipeline);

	void realtime_render();
	void offline_render(const std::string& name);

private:

	void recreateSwapChain();

	// For RealTime Renderer
	void updateDynamicSceneInfo(Timer& timer);

	// For Offline Renderer
	void updateRenderingSetting(Scene::SceneDynamicInfo dynamicInfo);

	// Rendering Info
	pstd::vector<VkStridedDeviceAddressRegionKHR> groupRegions;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	// Sync Object
	pstd::vector<Semaphore> renderFinishedSemaphores;
	pstd::vector<Semaphore> imageAvailableSemaphores;
	pstd::vector<Fence> inFlightFences;


	// Resource
	pstd::vector<CommandBuffer> renderCmdBuffers;
	pstd::vector<Image> hdrImages;
	pstd::vector<Image> ldrImages;
	pstd::vector<Buffer> uniformBuffers;
	pstd::vector<DescriptorSet*> dynamicSets;
	pstd::vector<DescriptorSet*> imageSets;
	pstd::vector<DescriptorSet*> toneMappingSets;
	std::unordered_map<std::string, DescriptorSet*> descriptorSets;
	std::unordered_map<std::string, ComputePipeline*> computePipelines;

	// Reference
	Window& window;
	SwapChain& swapChain;
	Scene& scene;
	Context& _context;

	// Else
	CameraController cameraController;
	TimerManager timerManager;

	uint32_t currentFrame = 0;
};



#endif // !PBRT_GRAPHICS_RENDERER

