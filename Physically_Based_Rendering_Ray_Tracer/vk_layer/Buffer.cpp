#include "Buffer.h"

Buffer::Buffer(VkDevice device, VkDeviceMemory memory, VkBuffer buffer, VkDeviceSize size): _device(device),_memory(memory),_buffer(buffer), size(size){}

Buffer::Buffer(Buffer&& other) noexcept :_device(other._device), _memory(other._memory), _buffer(other._buffer), size(other.size),map_address(other.map_address) {
	other._memory = VK_NULL_HANDLE;
	other._buffer = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;

	other.map_address = nullptr;
}
Buffer& Buffer::operator=(Buffer&& other) noexcept {
	if (&other != this) {
		if (map_address) unmap_memory();
		if (_buffer != VK_NULL_HANDLE)vkDestroyBuffer(_device, _buffer, nullptr);
		if (_memory != VK_NULL_HANDLE)vkFreeMemory(_device, _memory, nullptr);
	}

	_device = other._device;
	_memory = other._memory;
	_buffer = other._buffer;
	size = other.size;
	map_address = other.map_address;

	other._memory = VK_NULL_HANDLE;
	other._buffer = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
	other.map_address = nullptr;

	return *this;
}

VkDeviceAddress Buffer::device_address() const{
	VkBufferDeviceAddressInfo addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	addressInfo.buffer = _buffer;
	return vkGetBufferDeviceAddress(_device, &addressInfo);
}


Buffer::~Buffer() {
	if (map_address)unmap_memory();
	if (_buffer != VK_NULL_HANDLE)vkDestroyBuffer(_device, _buffer, nullptr);
	if (_memory != VK_NULL_HANDLE)vkFreeMemory(_device, _memory, nullptr);
}

void* Buffer::map_memory() {
	if (map_address) return map_address;
	vkMapMemory(_device, _memory, 0, size, 0, &map_address);
	return map_address;
}

void Buffer::unmap_memory() {
	if (!map_address)return;
	vkUnmapMemory(_device, _memory);
	map_address = nullptr;
}