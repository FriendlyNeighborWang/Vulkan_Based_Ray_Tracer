#ifndef PBRT_GRAPHICS_WINDOW_H
#define PBRT_GRAPHICS_WINDOW_H

#include "util/pstd.h"

#include <unordered_set>
#include <GLFW/glfw3.h>


class Window {
public:
	Window(uint32_t width, uint32_t height);

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	~Window();

	bool should_close() const { return glfwWindowShouldClose(window); }

	void poll_events() const { glfwPollEvents(); }

	static pstd::span<const char*> glfw_extension();

	operator GLFWwindow* () const { return window; }

	VkExtent2D extent() { return VkExtent2D{ _width, _height }; }
	
	// Outer interface

	void capture_cursor();
	
	void release_cursor();

	bool is_cursor_captured() const { return cursorCaptured; }


	// Signal Process
	Vector2f consume_mouse_delta();

	float consume_mouse_scroll_delta();

	bool is_key_pressed(int key) const;
	bool is_key_released(int key) const;

	bool is_mouse_button_clicked(int button) const;

	void end_frame();

private:

	static void windowResizeCallback(GLFWwindow* window, int width, int height);
	static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


	GLFWwindow* window = nullptr;
	uint32_t _width, _height;
	bool window_resized = false;

	double lastMouseX = 0.0, lastMouseY = 0.0;
	float mouseDeltaX = 0.0, mouseDeltaY = 0.0;
	bool firstMouse = true;

	bool cursorCaptured = false;

	float scrollDeltaY = 0.0f;

	std::unordered_set<int> releasedKeys;
	std::unordered_set<int> clickedMouseButton;
	
};

#endif // !PBRT_GRAPHICS_WINDOW_H

