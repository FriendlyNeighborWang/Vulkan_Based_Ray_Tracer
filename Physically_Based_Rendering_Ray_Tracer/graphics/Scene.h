#ifndef PBRT_GRAPHICS_MESH_H
#define PBRT_GRAPHICS_MESH_H


#include "base/base.h"
#include "util/pstd.h"
#include "util/Vec.h"
#include "vk_layer/VkMemoryAllocator.h"

struct Material {
	Vector4f baseColor{ 1.0f };
	Float metallic{ 0.0f };
	Float roughness{ 1.0f };
};

struct Geometry {
	uint32_t materialIndex;
	uint32_t firstVertex;
	uint32_t vertexCount;
	uint32_t firstIndex;
	uint32_t primitiveCount;
};

struct Mesh{
	uint32_t vertexCount;
	uint32_t firstVertex;

	uint32_t indexCount;
	uint32_t firstIndex;

	uint32_t firstGeometry;
	pstd::vector<Geometry> geometries;
};


struct MeshInstance {
	uint32_t meshIndex = 0;
	glm::mat4 transform = glm::mat4(1.0f);
	uint32_t instanceCustomIndex = 0;
	uint32_t instanceShaderBindingTableRecordOffset = 0;
};

struct Scene {
	Buffer& get_material_buffer(const VkMemoryAllocator& memAllocator);
	Buffer& get_geometry_buffer(const VkMemoryAllocator& memAllocator);

	Buffer& get_vertex_buffer(const VkMemoryAllocator& memAllocator);
	Buffer& get_index_buffer(const VkMemoryAllocator& memAllocator);
	Buffer& get_normal_buffer(const VkMemoryAllocator& memAllocator);

	pstd::vector<Material> materials;

	pstd::vector<Mesh> meshes;
	pstd::vector<MeshInstance> instances;

	pstd::vector<Vector3f> vertices;
	pstd::vector<uint32_t> indices;
	pstd::vector<Normal> normals;

private:
	Buffer materialBuffer; bool if_materialBuffer_aval = false;
	Buffer geometryBuffer; bool if_geometryBuffer_aval = false;

	Buffer vertexBuffer; bool if_vertexBuffer_aval = false;
	Buffer indexBuffer; bool if_indexBuffer_aval = false;
	Buffer normalBuffer; bool if_normalBuffer_aval = false;
};


#endif // !PBRT_GRAPHICS_MESH_H
