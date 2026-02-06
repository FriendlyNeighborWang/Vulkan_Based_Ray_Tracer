#include "vk_layer/SwapChain.h"
#include "vk_layer/Context.h"
#include "vk_layer/Image.h"
#include "util/math.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <limits>

SwapChain::SwapChain(Context& context, GLFWwindow* window) : _context(context) {
	recreate(window);
}

SwapChain::~SwapChain() {
	if (!_views.empty()) {
		for (auto imageView : _views)
			vkDestroyImageView(_context, imageView, nullptr);
	}
	
	if (swapChain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(_context, swapChain, nullptr);
}

void SwapChain::recreate(GLFWwindow* window) {
	vkDeviceWaitIdle(_context);

	//clean
	if (!_views.empty()) {
		for (auto imageView : _views)
			vkDestroyImageView(_context, imageView, nullptr);
	}

	VkSwapchainKHR last_swapchain = VK_NULL_HANDLE;
	if (swapChain != VK_NULL_HANDLE)
		last_swapchain = swapChain;

	_views.clear();
	_images.clear();
	_layouts.clear();

	// Create

	SwapChainSupportDetails swapChainSupport = get_swapchain_support_details(_context, _context.device_surface());

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	// Create SwapChain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = _context.device_surface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	uint32_t queueFamilyIndices[] = { _context.gc_queue().index, _context.present_queue().index };

	if (_context.gc_queue().index != _context.present_queue().index) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = last_swapchain;

	if (vkCreateSwapchainKHR(_context, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("SwapChain:: Failed to create swapchain");
	}
	
	if (last_swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(_context, last_swapchain, nullptr);

	// Get SwapChain Image & Create ImageView
	vkGetSwapchainImagesKHR(_context, swapChain, &imageCount, nullptr);
	_images.resize(imageCount);
	vkGetSwapchainImagesKHR(_context, swapChain, &imageCount, _images.data());

	_format = surfaceFormat.format;
	_extent = extent;

	_views.resize(imageCount);
	_layouts.resize(imageCount);
	for (uint32_t i = 0; i < _images.size(); ++i) {
		_views[i] = Image::create_imageview(_context, _images[i], _format, VK_IMAGE_ASPECT_COLOR_BIT);
		_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
	}
}



VkResult SwapChain::acquire_next_image(VkSemaphore semaphore, VkFence fence, uint32_t* imageIndex, uint32_t timeout) {
	return vkAcquireNextImageKHR(_context, swapChain, timeout, semaphore, fence, imageIndex);
}


bool SwapChain::check_device_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
	auto supportDetails = get_swapchain_support_details(device, surface);
	return !supportDetails.formats.empty() && !supportDetails.presentModes.empty();
}

void SwapChain::image_layout_transtion(uint32_t idx, CommandBuffer& cmdBuffer, VkImageLayout targetLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = _images[idx];
	barrier.oldLayout = _layouts[idx];
	barrier.newLayout = targetLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	_layouts[idx] = targetLayout;
}

VkResult SwapChain::present(uint32_t imageIndex, const pstd::vector<VkSemaphore>& waitSemaphores) {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = waitSemaphores.size();
	presentInfo.pWaitSemaphores = waitSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	return vkQueuePresentKHR(_context.present_queue(), &presentInfo);
}


SwapChain::SwapChainSupportDetails SwapChain::get_swapchain_support_details(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails supportDetails;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &supportDetails.capabilities);

	// surface format
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	supportDetails.formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, supportDetails.formats.data());

	// present mode
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	supportDetails.presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, supportDetails.presentModes.data());

	return supportDetails;
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const pstd::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const pstd::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = pstd::Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = pstd::Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}
