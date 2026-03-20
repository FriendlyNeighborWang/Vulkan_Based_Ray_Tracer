#include "Scene.h"

#include "vk_layer/Context.h"
#include "vk_layer/VkMemoryAllocator.h"


void Scene::register_skybox(const std::string& skybox_path) {
	skybox = SkyBox(skybox_path);
	staticInfo.ifHasSkyBox = 1;
}

Buffer& Scene::get_light_buffer(Context& context) {
	if (lightBuffer.is_aval()) return lightBuffer;

	if (lights.empty()) return placeholderBuffer(context);

	lightBuffer = context.memAllocator().create_buffer(
		lights.size() * sizeof(Light),
		sizeof(Light),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(lights.data(), lightBuffer);


	return lightBuffer;
}

Buffer& Scene::get_material_buffer(Context& context) {
	if (materialBuffer.is_aval()) return materialBuffer;

	materialBuffer = context.memAllocator().create_buffer(
		materials.size() * sizeof(Material),
		sizeof(Material),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(materials.data(), materialBuffer);

	return materialBuffer;
}

pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*> Scene::get_textures_and_samplers(Context& context) {
	using ReturnType = pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*>;
	if (!textures_list.empty()) return ReturnType(&textures_list, &samplers_list);

	if (textures.empty()) {
		textures.push_back(placeholderTextureData());
		samplers.push_back(SamplerData{});
	}

	samplers_list.reserve(samplers.size());
	for (const auto& samplerinfo : samplers) 
		samplers_list.emplace_back(context, samplerinfo);

	textures_list = context.memAllocator().create_textures(textures, samplers_list);

	return ReturnType(&textures_list, &samplers_list);
}

Buffer& Scene::get_geometry_buffer(Context& context) {
	if (geometryBuffer.is_aval())return geometryBuffer;

	struct GeometryStruct {
		uint32_t geometry_material_index;
		uint32_t firstVertex;
		uint32_t firstIndex;
		uint32_t primitiveCount;
		uint32_t lightIdx;
		uint32_t _padding[3];
	};

	pstd::vector<GeometryStruct> geometries;

	for (const auto& mesh : meshes) {
		for (const auto& geometry : mesh.geometries) {
			GeometryStruct geo;
			geo.geometry_material_index = geometry.materialIndex;
			geo.firstIndex = mesh.firstIndex + geometry.firstIndex;
			geo.firstVertex = mesh.firstVertex + geometry.firstVertex;
			geo.primitiveCount = geometry.primitiveCount;
			geo.lightIdx = geometry.lightIdx;

			geometries.push_back(geo);
		}
	}

	geometryBuffer = context.memAllocator().create_buffer(
		geometries.size() * sizeof(GeometryStruct),
		sizeof(GeometryStruct),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	
	context.memAllocator().copy_to_buffer_directly(geometries.data(), geometryBuffer);

	return geometryBuffer;
}

Buffer& Scene::get_vertex_buffer(Context& context) {
	if (vertexBuffer.is_aval())return vertexBuffer;

	vertexBuffer = context.memAllocator().create_buffer(
		vertices.size() * sizeof(Vector3f),
		sizeof(Vector3f),
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(vertices.data(), vertexBuffer);


	return vertexBuffer;
}

Buffer& Scene::get_index_buffer(Context& context) {
	if (indexBuffer.is_aval())return indexBuffer;

	indexBuffer = context.memAllocator().create_buffer(
		indices.size() * sizeof(uint32_t),
		sizeof(uint32_t),
		VK_FORMAT_R32_UINT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(indices.data(), indexBuffer);

	return indexBuffer;
}

Buffer& Scene::get_texcoord_buffer(Context& context) {
	if (texcoordBuffer.is_aval())return texcoordBuffer;
	
	if (!(staticInfo.bufferFlags & BUFFER_FLAG_HAS_TEXCOORDS)) return placeholderBuffer(context);

	texcoordBuffer = context.memAllocator().create_buffer(
		texcoords.size() * sizeof(Vector2f),
		sizeof(Vector2f),
		VK_FORMAT_R32G32_SFLOAT,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(texcoords.data(), texcoordBuffer);

	return texcoordBuffer;
}


Buffer& Scene::get_normal_buffer(Context& context) {
	if (normalBuffer.is_aval())return normalBuffer;

	if (!(staticInfo.bufferFlags & BUFFER_FLAG_HAS_NORMALS)) return placeholderBuffer(context);

	normalBuffer = context.memAllocator().create_buffer(
		normals.size() * sizeof(Normal),
		sizeof(Normal),
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(normals.data(), normalBuffer);

	return normalBuffer;
}

Buffer& Scene::get_tangent_buffer(Context& context) {
	if (tangentBuffer.is_aval()) return tangentBuffer;

	if (!(staticInfo.bufferFlags & BUFFER_FLAG_HAS_TANGENTS)) return placeholderBuffer(context);

	tangentBuffer = context.memAllocator().create_buffer(
		tangents.size() * sizeof(Vector4f),
		sizeof(Vector4f),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(tangents.data(), tangentBuffer);

	return tangentBuffer;
}





Buffer Scene::get_dynamic_scene_info(Context& context) {
	Buffer dynamic_scene_info_buffer;
	dynamic_scene_info_buffer = context.memAllocator().create_buffer(
		sizeof(SceneDynamicInfo),
		sizeof(SceneDynamicInfo),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	context.memAllocator().copy_to_buffer(
		&dynamicInfo,
		sizeof(dynamicInfo),
		dynamic_scene_info_buffer,
		true
	);

	return dynamic_scene_info_buffer;
}

Buffer& Scene::get_static_scene_info(Context& context) {
	if (staticSceneInfoBuffer.is_aval()) return staticSceneInfoBuffer;

	staticSceneInfoBuffer = context.memAllocator().create_buffer(
		sizeof(staticInfo),
		sizeof(staticInfo),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(&staticInfo, staticSceneInfoBuffer);

	return staticSceneInfoBuffer;
}

pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*> Scene::get_skybox_textures_and_samplers(Context& context) {
	return skybox.get_skybox_textures_and_samplers(context);
}


Buffer& Scene::placeholderBuffer(Context& context) {
	if (_placeholderBuffer.is_aval()) return _placeholderBuffer;

	_placeholderBuffer = context.memAllocator().create_buffer(
		sizeof(uint32_t),
		sizeof(uint32_t),
		VK_FORMAT_UNDEFINED,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	return _placeholderBuffer;
}

TextureData Scene::placeholderTextureData() {
	if (if_placeholderTextureData_aval) return _placeholderTextureData;

	static unsigned char dummyPixel[] = { 255,255,255,255 };
	
	_placeholderTextureData.width = 1; 
	_placeholderTextureData.height = 1; 
	_placeholderTextureData.channels = 4;
	_placeholderTextureData.size = 4;
	_placeholderTextureData.data = dummyPixel;
	_placeholderTextureData.format = VK_FORMAT_R8G8B8A8_UNORM;

	if_placeholderTextureData_aval = true;
	return _placeholderTextureData;
}