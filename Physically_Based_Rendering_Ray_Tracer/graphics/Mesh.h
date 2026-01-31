#ifndef PBRT_GRAPHICS_MESH_H
#define PBRT_GRAPHICS_MESH_H


#include "base/base.h"
#include "util/pstd.h"
#include "util/Vec.h"
#include "vk_layer/VkMemoryAllocator.h"

//#include "glm/glm.hpp"

struct Mesh{

	Buffer& get_vertex_buffer(const VkMemoryAllocator& memAllocator) ;
	Buffer& get_index_buffer(const VkMemoryAllocator& memAllocator) ;
	Buffer& get_normal_buffer(const VkMemoryAllocator& memAllocator) ;
	
	Buffer vertexBuffer; bool isVertexBufferReady = false;
	Buffer indexBuffer; bool isIndexBufferReady = false;
	Buffer normalBuffer; bool isNormalBufferReady = false;

	pstd::vector<Vector2f> texCoords;
	pstd::vector<Normal> normals;
	pstd::vector<uint32_t> indices;
	pstd::vector<Vector3f> vertices;
	
};

struct MeshInstance {
	uint32_t meshIndex = 0;
	glm::mat4 transform = glm::mat4(1.0f);
	uint32_t instanceCustomIndex = 0;
	uint32_t instanceShaderBindingTableRecordOffset = 0;
};

#endif // !PBRT_GRAPHICS_MESH_H
