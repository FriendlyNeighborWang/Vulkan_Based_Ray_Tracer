#ifndef PBRT_GRAPHICS_MESH_H
#define PBRT_GRAPHICS_MESH_H


#include "base/base.h"
#include "util/pstd.h"
#include "util/Vec.h"
#include "vk_layer/VkMemoryAllocator.h"

struct Light
{
	uint32_t lightType;

	Vector3f emission;
	float power;

	Vector3f position;	// Point & Spot
	Vector3f direction; // Directional & Spot
	float range;		// Point & Spot

	float innerConeAngle; // Spot
	float outerConeAngle;

	uint32_t geometryIndex;	// Mesh Light
	float area;				// Mesh Light
	glm::mat4 transform;	// Mesh Light
};

struct Material {
	uint32_t materialType;
	Vector3f emission{ 0.0f };
	Vector4f albedo{ 1.0f };
	Float metallic{ 0.0f };
	Float roughness{ 1.0f };
	Float _padding[2]{ 0.0f };
};

struct Geometry {
	uint32_t materialIndex;
	uint32_t firstVertex;
	uint32_t vertexCount;
	uint32_t firstIndex;
	uint32_t primitiveCount;
	uint32_t lightIdx;
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
	Buffer& get_light_buffer(Context& context);
	Buffer& get_material_buffer(Context& context);
	Buffer& get_geometry_buffer(Context& context);

	Buffer& get_vertex_buffer(Context& context);
	Buffer& get_index_buffer(Context& context);
	Buffer& get_normal_buffer(Context& context);

	void update_dynamic_scene_info();
	Buffer& get_dynamic_scene_info(Context& context);
	Buffer& get_static_scene_info(Context& context);

	pstd::vector<Material> materials;

	pstd::vector<Light> lights;

	pstd::vector<Mesh> meshes;
	pstd::vector<MeshInstance> instances;

	pstd::vector<Vector3f> vertices;
	pstd::vector<uint32_t> indices;
	pstd::vector<Normal> normals;

	struct SceneStaticInfo {
		// Light
		uint32_t lightCount = 0;
		float totalLightPower = 0.0f;
		uint32_t padding[2];
	};

	struct SceneDynamicInfo {
		// Camera
		uint32_t padding[4];
	};

	SceneStaticInfo staticInfo;
	SceneDynamicInfo dynamicInfo;

private:
	Buffer lightBuffer; bool if_lightBuffer_aval = false;
	Buffer materialBuffer; bool if_materialBuffer_aval = false;
	Buffer geometryBuffer; bool if_geometryBuffer_aval = false;

	Buffer vertexBuffer; bool if_vertexBuffer_aval = false;
	Buffer indexBuffer; bool if_indexBuffer_aval = false;
	Buffer normalBuffer; bool if_normalBuffer_aval = false;

	Buffer dynamicSceneInfoBuffer; bool if_dynamicSceneInfoBuffer_aval = false;
	Buffer staticSceneInfoBuffer; bool if_staticSceneInfoBuffer_aval = false;
};


#endif // !PBRT_GRAPHICS_MESH_H
