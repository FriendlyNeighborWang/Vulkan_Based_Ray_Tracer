#ifndef PBRT_GRAPHICS_RENDERER_H
#define PBRT_GRAPHICS_RENDERER_H

#include "base/base.h"
#include "util/Timer.h"
#include "InputManager.h"
#include "Window.h"

#include <unordered_map>


class Renderer {
public:
	Renderer(Context& context, Window& window, SwapChain& swapChain, Scene& scene);
	~Renderer();

	// void create_images();

	void realtime_render();
	void offline_render(const std::string& name);
	void original_render();

private:

	void recreateSwapChain();

	// For RealTime Renderer
	void updateDynamicSceneInfo(Timer& timer, Buffer& dynamicInfoBuffer);

	// For Offline Renderer
	void updateRenderingSetting(Scene::SceneDynamicInfo dynamicInfo);

	// Rendering Info
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	// Sync Object
	pstd::vector<Semaphore> renderFinishedSemaphores;
	pstd::vector<Semaphore> imageAvailableSemaphores;
	pstd::vector<Fence> inFlightFences;

	// Reference
	Window& window;
	SwapChain& swapChain;
	Scene& scene;
	Context& _context;

	// Else
	InputManager inputManager;
	TimerManager timerManager;

	uint32_t currentFrame = 0;
	uint32_t frame_idx = 0;
	uint32_t temporal_idx = 0;
};



#endif // !PBRT_GRAPHICS_RENDERER_H

