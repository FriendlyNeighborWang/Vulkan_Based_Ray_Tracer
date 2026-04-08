#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#include "base/base.h"

class Buffer {
	friend class VkMemoryAllocator;
public:
	Buffer() {}
	Buffer(Buffer&& other) noexcept;
	Buffer& operator=(Buffer&& other) noexcept;

	~Buffer();

	VkDeviceAddress device_address() const;

	void* map_memory();
	void unmap_memory();
	void write_buffer(const void* data, VkDeviceSize data_size = 0, VkDeviceSize offset = 0 );

	void barrier(CommandBuffer& cmdBuffer, VkAccessFlags nextAccess, VkPipelineStageFlags nextStage);


	operator const VkBuffer& () const { return _buffer; }
	bool is_aval() const { return _buffer != VK_NULL_HANDLE; }


	VkDeviceSize size;
	VkDeviceSize stride;
	VkFormat format;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags memoryProperties;

	VkAccessFlags currentAccessFlags = 0;
	VkPipelineStageFlags currentPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
private:
	Buffer(VkDevice device, VkDeviceMemory memory, VkBuffer buffer, VkDeviceSize size, VkDeviceSize stride, VkFormat format, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);


	void* map_address = nullptr;

	VkDeviceMemory _memory = VK_NULL_HANDLE;
	VkBuffer _buffer = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};

#endif // !VULKAN_BUFFER

