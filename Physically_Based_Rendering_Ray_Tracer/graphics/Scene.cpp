#include "Scene.h"

Buffer& Scene::get_material_buffer(const VkMemoryAllocator& memAllocator) {
	if (if_materialBuffer_aval) return materialBuffer;

	materialBuffer = memAllocator.create_buffer(
		materials.size() * sizeof(Material),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	memAllocator.copy_to_buffer_directly(materials.data(), materialBuffer);

	if_materialBuffer_aval = true;

	return materialBuffer;
}

Buffer& Scene::get_geometry_buffer(const VkMemoryAllocator& memAllocator) {
	if (if_geometryBuffer_aval)return geometryBuffer;

	struct GeometryStruct {
		uint32_t geometry_material_index;
		uint32_t firstVertex;
		uint32_t firstIndex;
		uint32_t _padding;
	};

	pstd::vector<GeometryStruct> geometries;

	for (const auto& mesh : meshes) {
		for (const auto& geometry : mesh.geometries) {
			GeometryStruct geo;
			geo.geometry_material_index = geometry.materialIndex;
			geo.firstIndex = mesh.firstIndex + geometry.firstIndex;
			geo.firstVertex = mesh.firstVertex + geometry.firstVertex;

			geometries.push_back(geo);
		}
	}

	geometryBuffer = memAllocator.create_buffer(
		geometries.size() * sizeof(GeometryStruct),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	
	memAllocator.copy_to_buffer_directly(geometries.data(), geometryBuffer);

	if_geometryBuffer_aval = true;

	return geometryBuffer;
}

Buffer& Scene::get_vertex_buffer(const VkMemoryAllocator& memAllocator) {
	if (if_vertexBuffer_aval)return vertexBuffer;

	vertexBuffer = memAllocator.create_buffer(
		vertices.size() * sizeof(Vector3f),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	memAllocator.copy_to_buffer_directly(vertices.data(), vertexBuffer);

	if_vertexBuffer_aval = true;

	return vertexBuffer;
}
Buffer& Scene::get_index_buffer(const VkMemoryAllocator& memAllocator) {
	if (if_indexBuffer_aval)return indexBuffer;

	indexBuffer = memAllocator.create_buffer(
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	memAllocator.copy_to_buffer_directly(indices.data(), indexBuffer);

	if_indexBuffer_aval = true;

	return indexBuffer;
}

Buffer& Scene::get_normal_buffer(const VkMemoryAllocator& memAllocator) {
	if (if_normalBuffer_aval)return normalBuffer;

	normalBuffer = memAllocator.create_buffer(
		normals.size() * sizeof(Normal),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	memAllocator.copy_to_buffer_directly(normals.data(), normalBuffer);
	
	if_normalBuffer_aval = true;

	return normalBuffer;
}
