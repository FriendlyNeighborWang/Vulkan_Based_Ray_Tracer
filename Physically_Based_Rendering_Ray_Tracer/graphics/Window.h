#ifndef PBRT_GRAPHICS_WINDOW_H
#define PBRT_GRAPHICS_WINDOW_H

#include "util/pstd.h"

#include <GLFW/glfw3.h>


class Window {
public:
	Window(uint32_t width, uint32_t height):_width(width),_height(height) {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(_width, _height, "Vulkan RayTracing", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, windowResizeCallback);
	}
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	~Window() {
		if (window)
			glfwDestroyWindow(window);
		glfwTerminate();
	}

	bool should_close() const {
		return glfwWindowShouldClose(window);
	}

	void poll_events() const { glfwPollEvents(); }

	static pstd::span<const char*> glfw_extension() {
		uint32_t glfwExtensionsCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
		
		return pstd::span<const char*>(glfwExtensions, glfwExtensionsCount);
	}

	operator GLFWwindow* () const {
		return window;
	}

	VkExtent2D extent() { VkExtent2D{ _width, _height }; }
	

private:

	static void windowResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		app->window_resized = true;
		app->_width = static_cast<uint32_t>(width);
		app->_height = static_cast<uint32_t>(height);
	}

	GLFWwindow* window = nullptr;
	uint32_t _width, _height;
	bool window_resized = false;
};

#endif // !PBRT_GRAPHICS_WINDOW_H

