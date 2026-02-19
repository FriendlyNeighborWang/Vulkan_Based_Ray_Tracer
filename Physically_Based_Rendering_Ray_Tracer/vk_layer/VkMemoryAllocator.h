#ifndef VULKAN_MEMORY_ALLOCATOR
#define VULKAN_MEMORY_ALLOCATOR

#include "base/base.h"
#include "util/pstd.h"

class VkMemoryAllocator {
public:
	VkMemoryAllocator(Context& context);
	~VkMemoryAllocator();

	void init();


	Image create_image(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const;

	void copy_to_image(CommandBuffer& cmdBuffer, Buffer& buffer, Image& image) const;

	void copy_to_image(CommandBuffer& cmdBuffer, Image& srcImage, Image& dstImage) const;

	void blit_image(CommandBuffer& cmdBuffer, VkImage srcImage, VkImageLayout srcLayout, VkExtent2D srcExtent, VkImage dstImage, VkImageLayout dstLayout, VkExtent2D dstExtent, VkFilter filter) const;

	pstd::vector<Texture> create_textures(const pstd::vector<TextureData>& infos, pstd::vector<Sampler>& samplers);


	Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const ;

	// From Host to Device (with Memory map)
	void copy_to_buffer(const void* data, VkDeviceSize size, Buffer& dstBuffer, bool keep_mapping = false) const;

	// From Device to Device
	void copy_to_buffer(CommandBuffer& cmdBuffer, Buffer& srcBuffer, Buffer& dstBuffer) const;

	// From Image
	void copy_to_buffer(CommandBuffer& cmdBuffer, Image& srcImage, Buffer& dstBuffer) const;

	// From Host to Device (with Staging buffer)
	void copy_to_buffer_directly(const void* data, Buffer& dstBuffer) const;

	void unmap_buffer(Buffer& buffer) const;



	// Memory Barrier
	void memory_barrier(CommandBuffer& cmdBuffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

private:
	VkDeviceMemory allocate_memory(VkMemoryRequirements memReq, VkMemoryPropertyFlags properties) const;

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	VkImageAspectFlags getAspectFlags(VkFormat format) const;


	Context& _context;
};

#endif // !VULKAN_MEMORY_ALLOCATOR

