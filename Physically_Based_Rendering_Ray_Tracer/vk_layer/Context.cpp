#include "vk_layer/Context.h"
#include "vk_layer/SwapChain.h"
#include "vk_layer/ShaderManager.h"
#include "vk_layer/VkMemoryAllocator.h"
#include "vk_layer/CommandPool.h"
#include "vk_layer/DescriptorManager.h"
#include "vk_layer/ASManager.h"
#include "vk_layer/RTPipeline.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <set>
#include <cstring>

Context::Context(){
	_shaderManager = std::make_unique<ShaderManager>(*this);
	_memAllocator = std::make_unique<VkMemoryAllocator>(*this);
	_cmdPool = std::make_unique<CommandPool>(*this);
	_asManager = std::make_unique<ASManager>(*this);
	_rtPipeline = std::make_unique<RTPipeline>(*this);
	_descriptorManager = std::make_unique<DescriptorManager>(*this);
}


// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	if (strcmp(pCallbackData->pMessageIdName, "VVL-DEBUG-PRINTF") == 0) {

		std::string message = pCallbackData->pMessage;
		//if(message.find("Internal Warning")==std::string::npos)
		std::cerr << "[Shader] " << message << std::endl;
	}else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cerr << "[Validation] " << pCallbackData->pMessage << std::endl;
	}

	

	return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator
) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

// Instance
void Context::add_instance_extension(const char* extension) {
	instance_extensions.push_back(extension);
}

void Context::enable_validation_layers() {
	if_enable_validation_layers = true;
	validation_layers = { "VK_LAYER_KHRONOS_validation" };
	if (!checkValidationLayerSupport())
		throw std::runtime_error("Context:: Validation Layer is not available");
}

void Context::create_instance() {
	if (instance != VK_NULL_HANDLE) return;

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan RayTracer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	if (!checkInstanceExtensionSupport())
		throw std::runtime_error("Context:: Instance extensions are not available");

	// debug_printf
	VkValidationFeatureEnableEXT enableFeatures[] = {
		VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
	};

	VkValidationFeaturesEXT validationFeatures{};
	if (if_enable_validation_layers) {
		validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		validationFeatures.pNext = nullptr;
		validationFeatures.enabledValidationFeatureCount = 1;
		validationFeatures.pEnabledValidationFeatures = enableFeatures;
		validationFeatures.disabledValidationFeatureCount = 0;
		validationFeatures.pDisabledValidationFeatures = nullptr;
	}

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
	createInfo.ppEnabledExtensionNames = instance_extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (if_enable_validation_layers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		createInfo.ppEnabledLayerNames = validation_layers.data();
		debugCreateInfo = populateDebugMessengerCreateInfo();
		
		debugCreateInfo.pNext = &validationFeatures;
		createInfo.pNext = &debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("Context:: Failed to create instance");

	if (if_enable_validation_layers) {
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = populateDebugMessengerCreateInfo();
		CreateDebugUtilsMessengerEXT(instance, &messengerCreateInfo, nullptr, &debugMessenger);
	}
}

// Surface
void Context::create_surface(GLFWwindow* window) {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("Context:: Failed to create surface");
}

// Physical Device
void Context::add_device_extension(const char* extension) {
	device_extensions.push_back(extension);
}

void Context::pick_physical_device() {
	if (physicalDevice != VK_NULL_HANDLE) return;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("Context:: Failed to find GPUs with Vulkan support");

	pstd::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (checkDeviceSuitable(device, surface)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Context:: Failed to find a suitable GPU");
}

// Logical Device
void Context::create_device() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	pstd::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsAndComputeFamily.value(),
		indices.presentFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	createInfo.ppEnabledExtensionNames = device_extensions.data();
	createInfo.pNext = &device_features;
	createInfo.pEnabledFeatures = nullptr;

	if (if_enable_validation_layers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		createInfo.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Context:: Failed to create device");
	}

	vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsAndComputeQueue.queue);
	graphicsAndComputeQueue.index = indices.graphicsAndComputeFamily.value();

	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue.queue);
	presentQueue.index = indices.presentFamily.value();
}

void Context::init() {
	_shaderManager->init();
	_memAllocator->init();
	_cmdPool->init(5);
	_descriptorManager->init();
	_asManager->init();
	_rtPipeline->init();
}

// Destructor
Context::~Context() {
	std::cout << std::endl;
	LOG_STREAM("Context") << "Begin to deconstruct:" << std::endl;

	_rtPipeline.reset();
	_asManager.reset();
	_descriptorManager.reset();
	_cmdPool.reset();
	_memAllocator.reset();
	_shaderManager.reset();

	if (device != VK_NULL_HANDLE) {
		vkDestroyDevice(device, nullptr);
	}
	if (surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}
	if (if_enable_validation_layers && debugMessenger != VK_NULL_HANDLE) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
	}

	LOG_STREAM("Context") << "Context has been deconstructed" << std::endl;
}

// Private helpers
bool Context::checkInstanceExtensionSupport() {
	uint32_t extensionsCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

	pstd::vector<VkExtensionProperties> availableExtensions(extensionsCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());

	for (const char* extensionName : instance_extensions) {
		bool found = false;
		for (const auto& extensionProperties : availableExtensions) {
			if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

bool Context::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	pstd::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validation_layers) {
		bool found = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

VkDebugUtilsMessengerCreateInfoEXT Context::populateDebugMessengerCreateInfo() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
	return createInfo;
}

bool Context::checkDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices = findQueueFamilies(device, surface);
	bool deviceExtensionsSupported = checkDeviceExtensionSupport(device);

	bool swapchainSupported = false;
	if (deviceExtensionsSupported) {
		swapchainSupported = SwapChain::check_device_swapchain_support(device, surface);
	}

	bool deviceFeaturesSupported = checkDeviceFeatureSupport(device);

	return indices.isComplete() &&
		deviceExtensionsSupported &&
		swapchainSupported &&
		deviceFeaturesSupported;
}

Context::QueueFamilyIndices Context::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	pstd::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
			(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.graphicsAndComputeFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) indices.presentFamily = i;

		if (indices.isComplete()) break;
	}

	return indices;
}

bool Context::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	pstd::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	for (const char* extension : device_extensions) {
		bool found = false;
		for (const auto& extensionProperty : availableExtensions) {
			if (strcmp(extension, extensionProperty.extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

bool Context::checkDeviceFeatureSupport(VkPhysicalDevice device) {
	vkGetPhysicalDeviceFeatures2(device, &device_features);
	return features_validator();
}

// Queue operator
Context::Queue::operator const VkQueue& () const { return queue; }

// Member Access implementations
Context::operator const VkInstance& () const { return instance; }
Context::operator const VkPhysicalDevice& () const { return physicalDevice; }
Context::operator const VkDevice& () const { return device; }

ShaderManager& Context::shaderManager() { return *_shaderManager; }
VkMemoryAllocator& Context::memAllocator() { return *_memAllocator; }
CommandPool& Context::cmdPool() { return *_cmdPool; }
DescriptorManager& Context::descriptorManager() { return *_descriptorManager; }
ASManager& Context::asManager() { return *_asManager; }
RTPipeline& Context::rtPipeline() { return *_rtPipeline; }

Context::Queue& Context::gc_queue() { return graphicsAndComputeQueue; }
Context::Queue& Context::present_queue() { return presentQueue; }
VkSurfaceKHR& Context::device_surface() { return surface; }

bool Context::QueueFamilyIndices::isComplete() const {
	return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
}
