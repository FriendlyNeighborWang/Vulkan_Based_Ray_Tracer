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

	VkDeviceAddress device_address();

	void* map_memory();
	void unmap_memory();

	operator const VkBuffer& () const { return _buffer; }

	VkDeviceSize size;
private:
	Buffer(VkDevice device, VkDeviceMemory memory, VkBuffer buffer, VkDeviceSize size);


	void* map_address = nullptr;

	VkDeviceMemory _memory = VK_NULL_HANDLE;
	VkBuffer _buffer = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};

#endif // !VULKAN_BUFFER

