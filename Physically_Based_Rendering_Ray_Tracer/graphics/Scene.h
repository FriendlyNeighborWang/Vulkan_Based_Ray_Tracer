#ifndef PBRT_GRAPHICS_SCENE_H
#define PBRT_GRAPHICS_SCENE_H


#include "base/base.h"
#include "util/pstd.h"
#include "util/Vec.h"
#include "vk_layer/Buffer.h"
#include "vk_layer/Texture.h"


struct SamplerData {
	VkFilter magFilter = VK_FILTER_LINEAR;
	VkFilter minFilter = VK_FILTER_LINEAR;
	VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

struct CameraData {
	Vector3f position{ 0.0f };
	float yfov = 0.7854f;
	Vector3f direction{ 0.0f };
	float znear = 0.f;
	Vector3f up{ 0.0f };
	float zfar = 1000.0f;

};

struct TextureData {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t channels = 0;
	uint32_t bits = 8;
	VkDeviceSize size;
	const unsigned char* data = nullptr;

	uint32_t samplerIdx;
	VkFormat format;
};

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

	uint32_t ifDelta = false;
	uint32_t _padding[3];
};

struct Material {
	uint32_t materialType;
	Vector3f emission{ 0.0f };
	Vector4f albedo{ 1.0f };
	Float metallic{ 1.0f };
	Float roughness{ 1.0f };
	int emissiveTexture = -1;
	int albedoTexture = -1;
	int metallicRoughnessTexture = -1;
	int normalTexture = -1;
	uint32_t alphaMode = ALPHA_MODE_OPAQUE;
	float alphaCutoff = 0.0;
	uint32_t doublesided = 1;
	uint32_t _padding[3];
};

struct Geometry {
	uint32_t materialIndex;
	uint32_t firstVertex;
	uint32_t vertexCount;
	uint32_t firstIndex;
	uint32_t primitiveCount;
	uint32_t lightIdx;
	bool if_opaque = true;
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
	Scene() = default;
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = default;
	Scene& operator=(Scene&&) = default;

	Buffer& get_light_buffer(Context& context);
	Buffer& get_material_buffer(Context& context);
	pstd::vector<Texture>& get_textures(Context& context);

	Buffer& get_geometry_buffer(Context& context);

	Buffer& get_vertex_buffer(Context& context);
	Buffer& get_index_buffer(Context& context);
	Buffer& get_texcoord_buffer(Context& context);
	Buffer& get_normal_buffer(Context& context);
	Buffer& get_tangent_buffer(Context& context);

	Buffer& get_dynamic_scene_info(Context& context);
	Buffer& get_static_scene_info(Context& context);

	pstd::vector<Material> materials;
	pstd::vector<TextureData> textures;
	pstd::vector<SamplerData> samplers;

	pstd::vector<Light> lights;

	pstd::vector<Mesh> meshes;
	pstd::vector<MeshInstance> instances;

	pstd::vector<Vector3f> vertices;
	pstd::vector<uint32_t> indices;
	pstd::vector<Vector2f> texcoords;
	pstd::vector<Normal> normals;
	pstd::vector<Vector4f> tangents;

	struct SceneStaticInfo {
		// Light
		uint32_t lightCount = 0;
		float totalLightPower = 0.0f;

		// Buffer availability bitmask
		uint32_t bufferFlags = 0;

		uint32_t padding;
	};

	struct SceneDynamicInfo {

		static constexpr uint32_t camera_data_offset() { return static_cast<uint32_t>(offsetof(SceneDynamicInfo, camera)); }
		static constexpr uint32_t samplers_per_pixel_offset() { return static_cast<uint32_t>(offsetof(SceneDynamicInfo, samples_per_pixel)); }
		static constexpr uint32_t iteration_depth_offset() { return static_cast<uint32_t>(offsetof(SceneDynamicInfo, iteration_depth)); }


		// Camera
		CameraData camera;

		// Ray Tracing Param
		uint32_t samples_per_pixel = 16;
		uint32_t iteration_depth = 4;
		uint32_t padding[2];
	};

	SceneStaticInfo staticInfo;
	SceneDynamicInfo dynamicInfo;

private:
	Buffer& placeholderBuffer(Context& context);


	Buffer lightBuffer; bool if_lightBuffer_aval = false;
	Buffer materialBuffer; bool if_materialBuffer_aval = false;
	pstd::vector<Texture> textures_list; bool if_textures_aval = false;
	pstd::vector<Sampler> samplers_list;
	
	Buffer geometryBuffer; bool if_geometryBuffer_aval = false;

	Buffer vertexBuffer; bool if_vertexBuffer_aval = false;
	Buffer indexBuffer; bool if_indexBuffer_aval = false;
	Buffer texcoordBuffer; bool if_texcoordBuffer_aval = false;
	Buffer normalBuffer; bool if_normalBuffer_aval = false;
	Buffer tangentBuffer; bool if_tangentBuffer_aval = false;
	

	Buffer dynamicSceneInfoBuffer; bool if_dynamicSceneInfoBuffer_aval = false;
	Buffer staticSceneInfoBuffer; bool if_staticSceneInfoBuffer_aval = false;

	Texture _placeholderTexture;
	Buffer _placeholderBuffer; bool if_placeHolderBuffer_aval = false;
};


#endif // !PBRT_GRAPHICS_SCENE_H
