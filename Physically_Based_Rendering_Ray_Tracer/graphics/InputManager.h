#ifndef PBRT_GRAPHICS_INPUT_MANAGER_H
#define PBRT_GRAPHICS_INPUT_MANAGER_H

#include "base/base.h"
#include "Scene.h"

class InputManager {
public:
	InputManager(Window& window, Scene::SceneDynamicInfo& dynamicInfo);

	void update(float delta);

	bool pressed_trigger(const pstd::vector<int>& keys) const;

	void release_cursor();

private:
	void updateCamera(float delta);

	void updateRenderingSettinges();

	void updateCursorState();


	Window& window;
	Scene::SceneDynamicInfo& dynamicInfo;

	float yaw;
	float pitch;
	float moveSpeed = 2.0f;
	float mouseSensitivity = 0.1f;

	float fovSensitivity = 2.0f;
	float minFovDeg = 10.0f;
	float maxFovDeg = 120.0f;
	

};

#endif // !PBRT_GRAPHICS_INPUT_MANAGER_H

