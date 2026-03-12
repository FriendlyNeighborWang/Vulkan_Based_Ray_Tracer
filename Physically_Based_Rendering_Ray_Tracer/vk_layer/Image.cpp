#include "vk_layer/Image.h"

#include "CommandPool.h"

#include <stdexcept>

Image::Image(Image&& other) noexcept: _device(other._device), _memory(other._memory), _image(other._image), _imageView(other._imageView), extent(other.extent), tiling(other.tiling), layout(other.layout), format(other.format), size(other.size), usage(other.usage), memoryProperties(other.memoryProperties), currentAccessFlags(other.currentAccessFlags), currentPipelineStage(other.currentPipelineStage), map_address(other.map_address) {
	other._device = VK_NULL_HANDLE;
	other._memory = VK_NULL_HANDLE;
	other._image = VK_NULL_HANDLE;
	other._imageView = VK_NULL_HANDLE;
	other.map_address = nullptr;
}
Image& Image::operator=(Image&& other) noexcept {
	if (this != &other) {
		if (_device != VK_NULL_HANDLE) {
			if (map_address)unmap_memory();
			if (_imageView != VK_NULL_HANDLE) vkDestroyImageView(_device, _imageView, nullptr);
			if (_image != VK_NULL_HANDLE) vkDestroyImage(_device, _image, nullptr);
			if (_memory != VK_NULL_HANDLE) vkFreeMemory(_device, _memory, nullptr);
		}

		_device = other._device;
		_memory = other._memory;
		_image = other._image;
		_imageView = other._imageView;
		extent = other.extent;
		tiling = other.tiling;
		layout = other.layout;
		format = other.format;
		size = other.size;
		usage = other.usage;
		memoryProperties = other.memoryProperties;
		currentAccessFlags = other.currentAccessFlags;
		currentPipelineStage = other.currentPipelineStage;
		map_address = other.map_address;

		other._device = VK_NULL_HANDLE;
		other._memory = VK_NULL_HANDLE;
		other._image = VK_NULL_HANDLE;
		other._imageView = VK_NULL_HANDLE;
		other.map_address = nullptr;
	}
	return *this;
}

VkImageView Image::create_imageview(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Image:: Failed to create Image View");

	return imageView;
}

void Image::transition_layout(Context& context, CommandBuffer& cmdBuffer, VkImageLayout targetLayout, VkAccessFlags nextAccess, VkPipelineStageFlags nextStage) {

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = _image;
	barrier.oldLayout = layout;
	barrier.newLayout = targetLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = currentAccessFlags;
	barrier.dstAccessMask = nextAccess;

	if (targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	vkCmdPipelineBarrier(
		cmdBuffer,
		currentPipelineStage, nextStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	layout = targetLayout;
	currentAccessFlags = nextAccess;
	currentPipelineStage = nextStage;
}

void Image::transition_layout(Context& context, CommandBuffer& cmdBuffer, VkImageLayout targetLayout) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = _image;
	barrier.oldLayout = layout;
	barrier.newLayout = targetLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	if (targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	vkCmdPipelineBarrier(
		cmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	layout = targetLayout;
}

void* Image::map_memory() {
	if (map_address)return map_address;
	vkMapMemory(_device, _memory, 0, size, 0, &map_address);
	return map_address;
}

void Image::unmap_memory() {
	if (!map_address)return;
	vkUnmapMemory(_device, _memory);
	map_address = nullptr;
}

bool Image::hasStencilComponent(VkFormat format){
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT;
}

Image::Image(VkDevice device, VkDeviceMemory memory, VkImage image, VkImageView imageView, VkExtent2D extent, VkImageTiling tiling, VkImageLayout layout, VkFormat format, VkDeviceSize size, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties) :_device(device), _memory(memory), _image(image), _imageView(imageView), extent(extent), tiling(tiling), layout(layout),format(format), size(size), usage(usage), memoryProperties(memoryProperties){}

Image::~Image() {
	if (map_address)unmap_memory();
	if (_imageView != VK_NULL_HANDLE)vkDestroyImageView(_device, _imageView, nullptr);
	if (_image != VK_NULL_HANDLE)vkDestroyImage(_device, _image, nullptr);
	if (_memory != VK_NULL_HANDLE)vkFreeMemory(_device, _memory, nullptr);
}