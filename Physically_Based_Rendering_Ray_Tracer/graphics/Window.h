#ifndef PBRT_GRAPHICS_WINDOW_H
#define PBRT_GRAPHICS_WINDOW_H

#include "util/pstd.h"

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

	Vector2f get_mouse_delta();

	bool is_key_pressed(int key) const;

	void process_input();


private:

	static void windowResizeCallback(GLFWwindow* window, int width, int height);

	static void mouseCallback(GLFWwindow* window, double xpos, double ypos);

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


	GLFWwindow* window = nullptr;
	uint32_t _width, _height;
	bool window_resized = false;

	double lastMouseX = 0.0, lastMouseY = 0.0;
	float mouseDeltaX = 0.0, mouseDeltaY = 0.0;
	bool cursorCaptured = false;
	bool firstMouse = true;
};

#endif // !PBRT_GRAPHICS_WINDOW_H

