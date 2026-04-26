#include "InputManager.h"

#include "Window.h"

InputManager::InputManager(Window& window, Scene::SceneDynamicInfo& dynamicInfo):window(window), dynamicInfo(dynamicInfo) {
	Vector3f dir = Normalize(Vector3f(
		dynamicInfo.camera.direction.x(),
		dynamicInfo.camera.direction.y(),
		dynamicInfo.camera.direction.z()
	));

	pitch = static_cast<float>(pstd::Degrees(asin(dir.y())));
	yaw = static_cast<float>(pstd::Degrees(atan2(dir.x(), dir.z())));
}

void InputManager::update(float delta) {
	updateCursorState();
	updateCamera(delta);
	updateRenderingSettinges();
	window.end_frame();
}

bool InputManager::pressed_trigger(const pstd::vector<int>& keys, bool if_release) const {
	if (keys.empty()) return false;

	if (if_release)
	{
		for (uint32_t i = 0; i < keys.size() - 1; ++i)
		{
			if (!window.is_key_pressed(keys[i])) return false;
		}
		return window.is_key_released(keys.back());
	}

	for (const auto& key : keys) 
		if (!window.is_key_pressed(key)) return false;

	return true;
}

void InputManager::release_cursor() {
	window.release_cursor();
}


void InputManager::updateCursorState() {
	if (window.is_cursor_captured() && window.is_key_released(GLFW_KEY_ESCAPE))
		window.release_cursor();

	if (!window.is_cursor_captured() && window.is_mouse_button_clicked(GLFW_MOUSE_BUTTON_LEFT))
		window.capture_cursor();
}

void InputManager::updateCamera(float delta) {
	if (!window.is_cursor_captured())return;

	CameraData& camera = dynamicInfo.camera;

	// View
	Vector2f mouseDelta = window.consume_mouse_delta();
	yaw -= mouseDelta.x() * mouseSensitivity;
	pitch -= mouseDelta.y() * mouseSensitivity;
	pitch = pstd::Clamp(pitch, -89.0f, 89.0f);

	float pitchRad = pstd::Radians(pitch);
	float yawRad = pstd::Radians(yaw);

	Vector3f direction;
	direction.x() = cos(pitchRad) * sin(yawRad);
	direction.y() = sin(pitchRad);
	direction.z() = cos(pitchRad) * cos(yawRad);
	direction = Normalize(direction);

	Vector3f worldUp(0.0f, 1.0f, 0.0f);
	Vector3f right = Normalize(Cross(direction, worldUp));
	Vector3f up = Normalize(Cross(right, direction));

	camera.direction = direction;
	camera.up = up;

	// FOV
	float scroll = window.consume_mouse_scroll_delta();
	if (scroll != 0.0f) {
		float fovDeg = static_cast<float>(pstd::Degrees(camera.yfov));
		fovDeg -= scroll * fovSensitivity;
		fovDeg = pstd::Clamp(fovDeg, minFovDeg, maxFovDeg);
		camera.yfov = static_cast<float>(pstd::Radians(fovDeg));
	}

	// Position
	Vector3f forward = Normalize(Vector3f(direction.x(), 0.0f, direction.z()));
	float speed = moveSpeed * delta;

	if (window.is_key_pressed(GLFW_KEY_LEFT_SHIFT))
		speed *= 4;

	if (window.is_key_pressed(GLFW_KEY_W) || window.is_key_pressed(GLFW_KEY_UP))
		camera.position += forward * speed;
	if (window.is_key_pressed(GLFW_KEY_S) || window.is_key_pressed(GLFW_KEY_DOWN))
		camera.position -= forward * speed;
	if (window.is_key_pressed(GLFW_KEY_A) || window.is_key_pressed(GLFW_KEY_LEFT))
		camera.position -= right * speed;
	if (window.is_key_pressed(GLFW_KEY_D) || window.is_key_pressed(GLFW_KEY_RIGHT))
		camera.position += right * speed;
	if (window.is_key_pressed(GLFW_KEY_SPACE))
		camera.position += worldUp * speed;
	if (window.is_key_pressed(GLFW_KEY_LEFT_CONTROL))
		camera.position -= worldUp * speed;
}

void InputManager::updateRenderingSettinges() {
	
	// Iteration Depth
	if (window.is_cursor_captured() && window.is_key_released(GLFW_KEY_E)) {
		dynamicInfo.iteration_depth = std::min(dynamicInfo.iteration_depth + 1, 8u);
		LOG_STREAM("InputManager") << "Iteration depth: " << dynamicInfo.iteration_depth << std::endl;
	}
	if (window.is_cursor_captured() && window.is_key_released(GLFW_KEY_C)) {
		dynamicInfo.iteration_depth = std::max(dynamicInfo.iteration_depth - 1, 1u);
		LOG_STREAM("InputManager") << "Iteration depth: " << dynamicInfo.iteration_depth << std::endl;
	}

	// Samples Per Pixel
	if (window.is_cursor_captured() && window.is_key_released(GLFW_KEY_Q)) {
		dynamicInfo.samples_per_pixel = std::min(dynamicInfo.samples_per_pixel + 4, 32u);
		LOG_STREAM("InputManager") << "Samples Per Pixel: " << dynamicInfo.samples_per_pixel << std::endl;
	}
	if (window.is_cursor_captured() && window.is_key_released(GLFW_KEY_Z)) {
		dynamicInfo.samples_per_pixel = std::max(dynamicInfo.samples_per_pixel - 4, 4u);
		LOG_STREAM("InputManager") << "Samples Per Pixel: " << dynamicInfo.samples_per_pixel << std::endl;
	}
}


