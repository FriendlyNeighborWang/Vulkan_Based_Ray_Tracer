#ifndef PBRT_GRAPHICS_SCENE_H
#define PBRT_GRAPHICS_SCENE_H


#include "base/base.h"
#include "util/pstd.h"
#include "util/Vec.h"
#include "SkyBox.h"
#include "vk_layer/Buffer.h"



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
	VkDeviceSize size = 0;
	const unsigned char* data = nullptr;

	uint32_t samplerIdx=0;
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
	Vector3f specular{ 1.0f };
	float metallic{ 1.0f };
	float roughness{ 1.0f };
	float transmissionFactor{ 0.0f };
	float specularFactor{ 1.0f };
	float ior{ 1.5f };
	int emissiveTexture = -1;
	int albedoTexture = -1;
	int metallicRoughnessTexture = -1;
	int normalTexture = -1;
	int transmissionTexture = -1;
	int specularTexture = -1;
	int specularColorTexture = -1;
	uint32_t alphaMode = ALPHA_MODE_OPAQUE;
	float alphaCutoff = 0.0;
	uint32_t doublesided = 1;
	uint32_t _padding[2];
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
	friend class SceneLoader;

	Scene() = default;
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = default;
	Scene& operator=(Scene&&) = default;

	void register_skybox(const std::string& skybox_path);

	Buffer& get_light_buffer(Context& context);
	Buffer& get_material_buffer(Context& context);
	pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*> get_textures_and_samplers(Context& context);

	Buffer& get_geometry_buffer(Context& context);

	Buffer& get_vertex_buffer(Context& context);
	Buffer& get_index_buffer(Context& context);
	Buffer& get_texcoord_buffer(Context& context);
	Buffer& get_normal_buffer(Context& context);
	Buffer& get_tangent_buffer(Context& context);


	Buffer get_dynamic_scene_info(Context& context);
	Buffer& get_static_scene_info(Context& context);

	pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*> get_skybox_textures_and_samplers(Context& context);

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

		// SkyBox
		uint32_t ifHasSkyBox = 0;
		float skyBoxPower = 0.0f;
		uint32_t _padding[3];
	};

	struct SceneDynamicInfo {

		static constexpr uint32_t camera_data_offset() { return static_cast<uint32_t>(offsetof(SceneDynamicInfo, camera)); }
		static constexpr uint32_t samplers_per_pixel_offset() { return static_cast<uint32_t>(offsetof(SceneDynamicInfo, samples_per_pixel)); }
		static constexpr uint32_t iteration_depth_offset() { return static_cast<uint32_t>(offsetof(SceneDynamicInfo, iteration_depth)); }


		// Camera
		CameraData camera;

		// VP Matrix
		glm::mat4 viewProj;
		glm::mat4 prevViewProj;

		// Ray Tracing Param
		uint32_t samples_per_pixel = 16;
		uint32_t iteration_depth = 8;
		uint32_t padding[2];
	};

	SceneStaticInfo staticInfo;
	SceneDynamicInfo dynamicInfo;

	Buffer& placeholderBuffer(Context& context);
	static TextureData placeholderTextureData();

	static glm::mat4 look_at(const Vector3f& eye, const Vector3f& target, const Vector3f& up);
	static glm::mat4 perspective(float fovY, float aspect, float zNear, float zFar);

private:
	
	SkyBox skybox;

	Buffer lightBuffer; 
	Buffer materialBuffer; 
	pstd::vector<Texture> textures_list; 
	pstd::vector<Sampler> samplers_list;
	
	Buffer geometryBuffer; 

	Buffer vertexBuffer;
	Buffer indexBuffer; 
	Buffer texcoordBuffer;
	Buffer normalBuffer; 
	Buffer tangentBuffer; 
	

	Buffer staticSceneInfoBuffer; 


	inline static TextureData _placeholderTextureData; inline static bool if_placeholderTextureData_aval = false;
	Buffer _placeholderBuffer; 
};


#endif // !PBRT_GRAPHICS_SCENE_H
