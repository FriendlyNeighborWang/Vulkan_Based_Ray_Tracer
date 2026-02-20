#ifndef PBRT_GRAPHICS_CAMERA_CONTROLLER_H
#define PBRT_GRAPHICS_CAMERA_CONTROLLER_H

#include "Window.h"
#include "Scene.h"
#include "util/math.h"

class CameraController {
public:
	CameraController(CameraData& camera) :camera(camera) {
		Vector3f dir = Normalize(Vector3f(camera.direction.x(), camera.direction.y(), camera.direction.z()));
		pitch = static_cast<float>(pstd::Degrees(asin(dir.y())));
		yaw = static_cast<float>(pstd::Degrees(atan2(dir.x(), dir.z())));
	}

	void update(Window& window, float delta) {
		if (!window.is_cursor_captured())return;

		// View
		Vector2f mouseDelta = window.get_mouse_delta();
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

		
		// Position
		Vector3f forward = Normalize(Vector3f(direction.x(), 0.0f, direction.z()));
		float speed = moveSpeed * delta;

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

private:
	CameraData& camera;
	float yaw;
	float pitch;

	float moveSpeed = 2.0f;
	float mouseSensitivity = 0.1f;
};



#endif // !PBRT_GRAPHICS_CAMERA_CONTROLLER_H

