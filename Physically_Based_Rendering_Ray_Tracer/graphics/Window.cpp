#include "Window.h"
#include "util/Vec.h"

Window::Window(uint32_t width, uint32_t height) :_width(width), _height(height) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(_width, _height, "Vulkan RayTracing", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, windowResizeCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

Window::~Window() {
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

pstd::span<const char*> Window::glfw_extension() {
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	return pstd::span<const char*>(glfwExtensions, glfwExtensionsCount);
}

void Window::capture_cursor() {
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	cursorCaptured = true;
	firstMouse = true;
}

void Window::release_cursor() {
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	cursorCaptured = false;
	mouseDeltaX = 0.0;
	mouseDeltaY = 0.0;
}

Vector2f Window::get_mouse_delta() {
	Vector2f delta{ mouseDeltaX, mouseDeltaY };
	mouseDeltaX = 0.0f;
	mouseDeltaY = 0.0f;
	return delta;
}

bool Window::is_key_pressed(int key) const {
	return glfwGetKey(window, key) == GLFW_PRESS;
}

void Window::process_input() {
	if (cursorCaptured && is_key_pressed(GLFW_KEY_ESCAPE))
		release_cursor();
}



void Window::windowResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	app->window_resized = true;
	app->_width = static_cast<uint32_t>(width);
	app->_height = static_cast<uint32_t>(height);
}


void Window::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

	if (!app->cursorCaptured) return;

	if (app->firstMouse) {
		app->lastMouseX = xpos;
		app->lastMouseY = ypos;
		app->firstMouse = false;
		return;
	}

	app->mouseDeltaX += static_cast<float>(xpos - app->lastMouseX);
	app->mouseDeltaY += static_cast<float>(ypos - app->lastMouseY);
	app->lastMouseX = xpos;
	app->lastMouseY = ypos;
}


void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!app->cursorCaptured && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		app->capture_cursor();
}