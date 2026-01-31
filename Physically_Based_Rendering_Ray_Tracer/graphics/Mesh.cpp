#include "Mesh.h"

Buffer& Mesh::get_vertex_buffer(const VkMemoryAllocator& memAllocator) {
	if (isVertexBufferReady)return vertexBuffer;

	vertexBuffer = memAllocator.create_buffer(
		sizeof(Vector3f) * vertices.size(),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	memAllocator.copy_to_buffer_directly(
		vertices.data(),
		vertexBuffer
	);
	isVertexBufferReady = true;

	return vertexBuffer;
}

Buffer& Mesh::get_index_buffer(const VkMemoryAllocator& memAllocator)  {
	if (isIndexBufferReady) return indexBuffer;

	indexBuffer = memAllocator.create_buffer(
		sizeof(uint32_t) * indices.size(),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	memAllocator.copy_to_buffer_directly(
		indices.data(),
		indexBuffer
	);
	isIndexBufferReady = true;

	return indexBuffer;
}

Buffer& Mesh::get_normal_buffer(const VkMemoryAllocator& memAllocator)  {
	if (isNormalBufferReady)return normalBuffer;

	normalBuffer = memAllocator.create_buffer(
		sizeof(Normal) * normals.size(),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	memAllocator.copy_to_buffer_directly(
		normals.data(),
		normalBuffer
	);
	isNormalBufferReady = true;

	return normalBuffer;
}