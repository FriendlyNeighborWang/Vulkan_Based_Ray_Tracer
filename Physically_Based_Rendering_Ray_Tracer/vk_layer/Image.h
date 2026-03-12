#ifndef VULKAN_IMAGE
#define VULKAN_IMAGE

#include "base/base.h"

class Image {
	friend class VkMemoryAllocator;
public:
	Image(){}
	Image(Image&& other) noexcept;
	Image& operator=(Image&& other) noexcept;

	~Image();

	static VkImageView create_imageview(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void transition_layout(Context& context, CommandBuffer& cmdBuffer, VkImageLayout targetLayout, VkAccessFlags nextAccess,  VkPipelineStageFlags nextStage);

	void transition_layout(Context& context, CommandBuffer& cmdBuffer, VkImageLayout targetLayout);

	void* map_memory();
	void unmap_memory();

	static bool hasStencilComponent(VkFormat format);

	operator VkImage() const { return _image; }
	operator VkImageView() const { return _imageView; }
	bool is_aval() const { return _image != VK_NULL_HANDLE; }

	VkExtent2D extent;
	VkImageTiling tiling;
	VkImageLayout layout;
	VkFormat format;
	VkDeviceSize size;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags memoryProperties;

	VkAccessFlags currentAccessFlags = 0;
	VkPipelineStageFlags currentPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	
private:
	Image(VkDevice device, VkDeviceMemory memory, VkImage image, VkImageView imageView, VkExtent2D extent, VkImageTiling tiling, VkImageLayout layout, VkFormat format, VkDeviceSize size, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties);

	void* map_address = nullptr;

	VkDeviceMemory _memory = VK_NULL_HANDLE;
	VkImage _image = VK_NULL_HANDLE;
	VkImageView _imageView = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};

#endif // !VULKAN_IMAGE
