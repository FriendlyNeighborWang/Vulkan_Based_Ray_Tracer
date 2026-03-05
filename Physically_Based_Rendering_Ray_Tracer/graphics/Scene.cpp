#include "Scene.h"

#include "vk_layer/Context.h"
#include "vk_layer/VkMemoryAllocator.h"
#include "SkyBox.h"

Buffer& Scene::get_light_buffer(Context& context, SkyBox& skybox) {
	if (if_lightBuffer_aval) return lightBuffer;

	// Add Sky Box
	Light skyboxlight;
	if (float power = skybox.get_total_power(); power > 0.0f) {
		skyboxlight.lightType = LIGHT_TYPE_SKYBOX;
		skyboxlight.power = power;
		skyboxlight.ifDelta = false;

		staticInfo.lightCount += 1;
		staticInfo.totalLightPower += power;
		staticInfo.ifHasSkyBox = 1;
	}
	lights.push_back(skyboxlight);

	lightBuffer = context.memAllocator().create_buffer(
		lights.size() * sizeof(Light),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(lights.data(), lightBuffer);

	if_lightBuffer_aval = true;

	return lightBuffer;
}

Buffer& Scene::get_material_buffer(Context& context) {
	if (if_materialBuffer_aval) return materialBuffer;

	materialBuffer = context.memAllocator().create_buffer(
		materials.size() * sizeof(Material),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(materials.data(), materialBuffer);

	if_materialBuffer_aval = true;

	return materialBuffer;
}

pstd::vector<Texture>& Scene::get_textures(Context& context) {
	if (if_textures_aval) return textures_list;

	if (textures.empty()) {
		textures.push_back(placeholderTextureData());
		samplers.push_back(SamplerData{});
	}

	samplers_list.reserve(samplers.size());
	for (const auto& samplerinfo : samplers) 
		samplers_list.emplace_back(context, samplerinfo);

	textures_list = context.memAllocator().create_textures(textures, samplers_list);

	if_textures_aval = true;

	return textures_list;
}

Buffer& Scene::get_geometry_buffer(Context& context) {
	if (if_geometryBuffer_aval)return geometryBuffer;

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
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	
	context.memAllocator().copy_to_buffer_directly(geometries.data(), geometryBuffer);

	if_geometryBuffer_aval = true;

	return geometryBuffer;
}

Buffer& Scene::get_vertex_buffer(Context& context) {
	if (if_vertexBuffer_aval)return vertexBuffer;

	vertexBuffer = context.memAllocator().create_buffer(
		vertices.size() * sizeof(Vector3f),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(vertices.data(), vertexBuffer);

	if_vertexBuffer_aval = true;

	return vertexBuffer;
}

Buffer& Scene::get_index_buffer(Context& context) {
	if (if_indexBuffer_aval)return indexBuffer;

	indexBuffer = context.memAllocator().create_buffer(
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(indices.data(), indexBuffer);

	if_indexBuffer_aval = true;

	return indexBuffer;
}

Buffer& Scene::get_texcoord_buffer(Context& context) {
	if (if_texcoordBuffer_aval)return texcoordBuffer;
	
	if (!(staticInfo.bufferFlags & BUFFER_FLAG_HAS_TEXCOORDS)) return placeholderBuffer(context);

	texcoordBuffer = context.memAllocator().create_buffer(
		texcoords.size() * sizeof(Vector2f),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(texcoords.data(), texcoordBuffer);

	if_texcoordBuffer_aval = true;

	return texcoordBuffer;
}


Buffer& Scene::get_normal_buffer(Context& context) {
	if (if_normalBuffer_aval)return normalBuffer;

	if (!(staticInfo.bufferFlags & BUFFER_FLAG_HAS_NORMALS)) return placeholderBuffer(context);

	normalBuffer = context.memAllocator().create_buffer(
		normals.size() * sizeof(Normal),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(normals.data(), normalBuffer);
	
	if_normalBuffer_aval = true;

	return normalBuffer;
}

Buffer& Scene::get_tangent_buffer(Context& context) {
	if (if_tangentBuffer_aval) return tangentBuffer;

	if (!(staticInfo.bufferFlags & BUFFER_FLAG_HAS_TANGENTS)) return placeholderBuffer(context);

	tangentBuffer = context.memAllocator().create_buffer(
		tangents.size() * sizeof(Vector4f),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(tangents.data(), tangentBuffer);

	if_tangentBuffer_aval = true;

	return tangentBuffer;
}





Buffer Scene::get_dynamic_scene_info(Context& context) {
	Buffer dynamic_scene_info_buffer;
	dynamic_scene_info_buffer = context.memAllocator().create_buffer(
		sizeof(SceneDynamicInfo),
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
	if (if_staticSceneInfoBuffer_aval) return staticSceneInfoBuffer;

	staticSceneInfoBuffer = context.memAllocator().create_buffer(
		sizeof(staticInfo),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	context.memAllocator().copy_to_buffer_directly(&staticInfo, staticSceneInfoBuffer);

	if_staticSceneInfoBuffer_aval = true;

	return staticSceneInfoBuffer;
}



Buffer& Scene::placeholderBuffer(Context& context) {
	if (if_placeHolderBuffer_aval) return _placeholderBuffer;

	_placeholderBuffer = context.memAllocator().create_buffer(
		sizeof(uint32_t),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	if_placeHolderBuffer_aval = true;

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