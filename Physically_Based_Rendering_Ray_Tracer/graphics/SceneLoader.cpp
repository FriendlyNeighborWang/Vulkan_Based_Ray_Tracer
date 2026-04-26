#include "SceneLoader.h"




Scene SceneLoader::LoadScene(const std::string& filePath) {

	std::string err;
	std::string warn;

	loader.SetPreserveImageChannels(false);
	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);

	if (!warn.empty()) {
		LOG_STREAM("SceneLoader") << "Warn: " << warn << std::endl;
	}
	if (!err.empty()) {
		LOG_STREAM("SceneLoader") << "Error: " << err << std::endl;
	}
	if (!res)
		throw std::runtime_error("SceneLoader::Failed to load scene");

	Scene outScene;

	loadTexture(outScene);

	loadMaterial(outScene);

	loadMeshes(outScene);

	loadCamera(outScene);

	loadPunctualLights(outScene);

	const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

	glm::mat4 rootTransform(1.0f);
	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		processNode(model.nodes[scene.nodes[i]], rootTransform, outScene);
	}

	loadMeshLights(outScene);



	LOG_STREAM("SceneLoader") << " Materials: " << outScene.materials.size() << std::endl;
	LOG_STREAM("SceneLoader") << " Textures: " << outScene.textures.size() << std::endl;
	LOG_STREAM("SceneLoader") << " Meshes (BLAS): " << outScene.meshes.size() << std::endl;
	LOG_STREAM("SceneLoader") << " Instances (TLAS): " << outScene.instances.size() << std::endl;
	LOG_STREAM("SceneLoader") << " Lights: " << outScene.lights.size() << std::endl;
	LOG_STREAM("SceneLoader") << " Camera: (" << outScene.dynamicInfo.camera.position.x() << ", " << outScene.dynamicInfo.camera.position.y() << ", " << outScene.dynamicInfo.camera.position.z() << ")" << std::endl;


	return outScene;
}


void SceneLoader::loadTexture(Scene& scene) {
	if (model.textures.empty()) return;

	scene.textures.reserve(model.textures.size());

	for (const auto& gltfTex : model.textures) {
		TextureData tex{};

		if (gltfTex.source >= 0 && gltfTex.source < static_cast<uint32_t>(model.images.size())) {
			const tinygltf::Image& img = model.images[gltfTex.source];
			tex.width = static_cast<uint32_t>(img.width);
			tex.height = static_cast<uint32_t>(img.height);
			tex.channels = static_cast<uint32_t>(img.component);
			tex.data = img.image.data();
			tex.bits = img.bits;
			tex.size = tex.width * tex.height * tex.channels * img.bits / 8;
			tex.samplerIdx = gltfTex.sampler >= 0 ? static_cast<uint32_t>(gltfTex.sampler) : 0;
		}

		scene.textures.push_back(tex);
	}

	scene.samplers.reserve(model.samplers.size());

	for (const auto& gltfSampler : model.samplers) {
		SamplerData sam{};
		sam.magFilter = glFilterToVk(gltfSampler.magFilter);
		sam.minFilter = glFilterToVk(gltfSampler.minFilter);
		sam.addressModeU = glWrapToVk(gltfSampler.wrapS);
		sam.addressModeV = glWrapToVk(gltfSampler.wrapT);

		scene.samplers.push_back(sam);
	}

	if (scene.samplers.empty())
		scene.samplers.push_back(SamplerData{});

}


void SceneLoader::loadMaterial(Scene& scene) {
	if (model.materials.empty()) return;

	scene.materials.reserve(model.materials.size());

	for (const auto& mat : model.materials) {
		Material material{};
		material.materialType = MATERIAL_TYPE_COOK_TORRANCE;

		// Albedo
		if (mat.values.find("baseColorFactor") != mat.values.end()) {
			const auto& color = mat.values.at("baseColorFactor").ColorFactor();
			material.albedo = Vector4f((float)color[0], (float)color[1], (float)color[2], (float)color[3]);
		}
		material.albedoTexture = mat.pbrMetallicRoughness.baseColorTexture.index;
		if (material.albedoTexture != -1) {
			auto& tex = scene.textures[material.albedoTexture];
			tex.format = findTextureFormat(tex, true);
		}

		// Metallic & Roughness
		if (mat.values.find("metallicFactor") != mat.values.end()) {
			material.metallic = (float)mat.values.at("metallicFactor").Factor();
		}
		if (mat.values.find("roughnessFactor") != mat.values.end()) {
			material.roughness = (float)mat.values.at("roughnessFactor").Factor();
		}
		material.metallicRoughnessTexture = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
		if (material.metallicRoughnessTexture != -1) {
			auto& tex = scene.textures[material.metallicRoughnessTexture];
			tex.format = findTextureFormat(tex, false);
		}


		// Emission
		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
			const auto& emissive = mat.additionalValues.at("emissiveFactor").ColorFactor();
			float strength = 1.0f;
			if (auto extIt = mat.extensions.find("KHR_materials_emissive_strength");
				extIt != mat.extensions.end() && extIt->second.Has("emissiveStrength")) {
				strength = static_cast<float>(extIt->second.Get("emissiveStrength").GetNumberAsDouble());
			}

			material.emission = Vector3f(float(emissive[0]) * strength, float(emissive[1]) * strength, float(emissive[2]) * strength);
		}

		material.emissiveTexture = mat.emissiveTexture.index;

		if (material.emissiveTexture != -1) {
			auto& tex = scene.textures[material.emissiveTexture];
			tex.format = findTextureFormat(tex, true);

			if (material.emission == Vector3f(0.0f))
				material.emission = Vector3f(1.0f);
		}


		// Normal
		material.normalTexture = mat.normalTexture.index;
		if (material.normalTexture != -1) {
			auto& tex = scene.textures[material.normalTexture];
			tex.format = findTextureFormat(tex, false);
		}


		// Alpha
		if (mat.alphaMode == "MASK") {
			material.alphaMode = ALPHA_MODE_MASK;
			material.alphaCutoff = mat.alphaCutoff;
		}
		else if (mat.alphaMode == "BLEND") {
			material.alphaMode = ALPHA_MODE_BLEND;
		}
		else {
			material.alphaMode = ALPHA_MODE_OPAQUE;
		}


		// Double Sided
		if (mat.doubleSided)
			material.doublesided = 1;
		else
			material.doublesided = 0;


		// Transimission 
		if (auto extIt = mat.extensions.find("KHR_materials_transmission"); extIt != mat.extensions.end()) {
			const auto& transmission = extIt->second;

			if (transmission.Has("transmissionFactor")) {
				material.transmissionFactor = static_cast<float>(transmission.Get("transmissionFactor").GetNumberAsDouble());
			}
			if (transmission.Has("transmissionTexture")) {
				material.transmissionTexture = transmission.Get("transmissionTexture").Get("index").GetNumberAsInt();
				if (material.transmissionTexture != -1) {
					auto& tex = scene.textures[material.transmissionTexture];
					tex.format = findTextureFormat(tex, false);
				}
			}
		}

		// IOR
		if (auto extIt = mat.extensions.find("KHR_materials_ior"); extIt != mat.extensions.end()) {
			const auto& ior = extIt->second;
			if (ior.Has("ior")) {
				material.ior = static_cast<float>(ior.Get("ior").GetNumberAsDouble());
			}
		}

		// Specular 
		if (auto extIt = mat.extensions.find("KHR_materials_specular"); extIt != mat.extensions.end()) {
			const auto& specular = extIt->second;
			if (specular.Has("specularFactor")) {
				material.specularFactor = static_cast<float>(specular.Get("specularFactor").GetNumberAsDouble());
			}

			if (specular.Has("specularColorFactor")) {
				const auto& arr = specular.Get("specularColorFactor");
				if (arr.ArrayLen() == 3) {
					material.specular = Vector3f(
						static_cast<float>(arr.Get(0).GetNumberAsDouble()),
						static_cast<float>(arr.Get(1).GetNumberAsDouble()),
						static_cast<float>(arr.Get(2).GetNumberAsDouble())
					);
				}
			}

			if (specular.Has("specularTexture")) {
				material.specularTexture = specular.Get("specularTexture").Get("index").GetNumberAsInt();
				if (material.specularTexture != -1) {
					auto& tex = scene.textures[material.specularTexture];
					tex.format = findTextureFormat(tex, false);
				}
			}
			
			if (specular.Has("specularColorTexture")) {
				material.specularColorTexture = specular.Get("specularColorTexture").Get("index").GetNumberAsInt();
				if (material.specularColorTexture != -1) {
					auto& tex = scene.textures[material.specularColorTexture];
					tex.format = findTextureFormat(tex, true);
				}
			}

		}


		scene.materials.push_back(material);
	}
}


void SceneLoader::loadMeshes(Scene& scene) {
	if (model.meshes.empty())return;

	uint32_t geometryCount = 0;

	scene.meshes.reserve(model.meshes.size());

	for (const auto& gltfMesh : model.meshes) {
		Mesh mesh{};

		mesh.firstVertex = static_cast<uint32_t>(scene.vertices.size());
		mesh.firstIndex = static_cast<uint32_t>(scene.indices.size());
		mesh.firstGeometry = geometryCount;

		uint32_t meshVertexCount = 0;
		uint32_t meshIndexCount = 0;

		for (const auto& primitive : gltfMesh.primitives) {
			Geometry geometry{};

			geometry.materialIndex = primitive.material > -1 ? primitive.material : 0;
			geometry.firstVertex = meshVertexCount;
			geometry.firstIndex = meshIndexCount;


			// 
			const float* positionBuffer = nullptr;
			const float* normalBuffer = nullptr;
			const float* tangentBuffer = nullptr;
			const float* texcoordBuffer = nullptr;

			size_t vertexCount = 0;

			if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
				const Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
				const BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				positionBuffer = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
				vertexCount = accessor.count;
			}

			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				scene.staticInfo.bufferFlags |= BUFFER_FLAG_HAS_NORMALS;

				const Accessor& accessor = model.accessors[primitive.attributes.at("NORMAL")];
				const BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				normalBuffer = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
			}

			if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
				scene.staticInfo.bufferFlags |= BUFFER_FLAG_HAS_TANGENTS;

				const Accessor& accessor = model.accessors[primitive.attributes.at("TANGENT")];
				const BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				tangentBuffer = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
			}

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				scene.staticInfo.bufferFlags |= BUFFER_FLAG_HAS_TEXCOORDS;

				const Accessor& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
				const BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				texcoordBuffer = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
			}

			for (size_t v = 0; v < vertexCount; ++v) {
				Vector3f pos = { positionBuffer[v * 3 + 0], positionBuffer[v * 3 + 1], positionBuffer[v * 3 + 2] };
				Normal norm = normalBuffer ? Normal(normalBuffer[v * 3 + 0], normalBuffer[v * 3 + 1], normalBuffer[v * 3 + 2]) : Normal{ 0.0,0.0,0.0 };
				Vector4f tan = tangentBuffer ? Vector4f(tangentBuffer[v * 4 + 0], tangentBuffer[v * 4 + 1], tangentBuffer[v * 4 + 2], tangentBuffer[v * 4 + 3]) : Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
				Vector2f uv = texcoordBuffer ? Vector2f(texcoordBuffer[v * 2 + 0], texcoordBuffer[v * 2 + 1]) : Vector2f(0.0f, 0.0f);


				scene.vertices.push_back(pos);
				scene.normals.push_back(norm);
				scene.tangents.push_back(tan);
				scene.texcoords.push_back(uv);
			}


			uint32_t indexCount = 0;
			if (primitive.indices > -1) {
				const Accessor& accessor = model.accessors[primitive.indices];
				const BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void* dataPtr = &(buffer.data[accessor.byteOffset + view.byteOffset]);

				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; ++index)
						scene.indices.push_back(buf[index]);
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; ++index)
						scene.indices.push_back(buf[index]);

					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; ++index)
						scene.indices.push_back(buf[index]);
					break;
				}
				default:
					std::cerr << "Index component type not supported" << std::endl;
					return;
				}
			}

			geometry.vertexCount = static_cast<uint32_t>(vertexCount);
			geometry.primitiveCount = indexCount / 3;

			if (auto material = scene.materials[geometry.materialIndex];
				material.alphaMode != ALPHA_MODE_OPAQUE || 
				(material.transmissionFactor != 0.0 || material.transmissionTexture != -1)
				)
				geometry.if_opaque = false;
			else
				geometry.if_opaque = true;

			mesh.geometries.push_back(geometry);

			meshVertexCount += static_cast<uint32_t>(vertexCount);
			meshIndexCount += indexCount;
		}

		mesh.vertexCount = meshVertexCount;
		mesh.indexCount = meshIndexCount;
		geometryCount += static_cast<uint32_t>(mesh.geometries.size());
		scene.meshes.push_back(mesh);
	}
}


void SceneLoader::loadPunctualLights(Scene& scene) {
	// Load Punctual Light
	if (!model.lights.empty()) {
		for (const auto& gltfLight : model.lights) {

			Light light{};

			Vector3f color{ 1.0f };
			if (gltfLight.color.size() == 3) {
				color = Vector3f(
					static_cast<float>(gltfLight.color[0]),
					static_cast<float>(gltfLight.color[1]),
					static_cast<float>(gltfLight.color[2])
				);
			}
			float intensity = static_cast<float>(gltfLight.intensity) / 683.0f;
			light.emission = color * intensity;
			light.range = static_cast<float>(gltfLight.range);


			if (gltfLight.type == "directional") {
				light.lightType = LIGHT_TYPE_DIRECTIONAL;
				light.ifDelta = 1;
				light.power = computeLightPower(light);
			}
			else if (gltfLight.type == "point") {
				light.lightType = LIGHT_TYPE_POINT;
				light.ifDelta = 1;
				light.power = computeLightPower(light);
			}
			else if (gltfLight.type == "spot") {
				light.lightType = LIGHT_TYPE_SPOT;
				light.ifDelta = 1;
				light.innerConeAngle = static_cast<float>(gltfLight.spot.innerConeAngle);
				light.outerConeAngle = static_cast<float>(gltfLight.spot.outerConeAngle);
				light.power = computeLightPower(light);
			}
			else {
				continue;
			}

			scene.lights.push_back(light);
			scene.staticInfo.lightCount++;
			scene.staticInfo.totalLightPower += light.power;

		}
	}
}


void SceneLoader::loadMeshLights(Scene& scene) {
	uint32_t lightIdx = scene.lights.size();


	// Load Mesh Light
	for (const auto& instance : scene.instances) {
		const Mesh& mesh = scene.meshes[instance.meshIndex];
		uint32_t geoBase = mesh.firstGeometry;

		for (uint32_t localGeoIdx = 0; localGeoIdx < mesh.geometries.size(); ++localGeoIdx) {
			auto& geo = scene.meshes[instance.meshIndex].geometries[localGeoIdx];

			if (const Material& mat = scene.materials[geo.materialIndex]; mat.emission.norm() > 0.001f) {
				geo.lightIdx = lightIdx++;

				Light light{};
				light.lightType = LIGHT_TYPE_MESH;
				light.emission = mat.emission;
				light.geometryIndex = geoBase + localGeoIdx;
				light.transform = instance.transform;
				light.area = computeMeshLightArea(scene, scene.meshes[instance.meshIndex], geo, light);
				light.power = computeLightPower(light);
				light.ifDelta = 0;

				scene.lights.push_back(light);
				scene.staticInfo.lightCount++;
				scene.staticInfo.totalLightPower += light.power;
			}
		}
	}

}


void SceneLoader::loadCamera(Scene& scene) {

	CameraData cam{};

	for (const auto& gltfCam : model.cameras) {

		cam.direction = Vector3f(0.0f, 0.0f, -1.0f);
		cam.up = Vector3f(0.0f, 1.0f, 0.0f);

		if (gltfCam.type == "perspective") {
			cam.yfov = static_cast<float>(gltfCam.perspective.yfov);
			cam.znear = static_cast<float>(gltfCam.perspective.znear);
			cam.zfar = static_cast<float>(gltfCam.perspective.zfar);
		}
		scene.dynamicInfo.camera = cam;
	}


	if (model.cameras.size() == 0)
		scene.dynamicInfo.camera = cam;
}

void SceneLoader::processNode(const Node& node, const glm::mat4& parentTransform, Scene& scene) {
	glm::mat4 localTransform(1.0f);

	if (node.matrix.size() == 16) {
		for (uint32_t i = 0; i < 16; ++i)
			localTransform[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
	}
	else {
		glm::mat4 T(1.0f), R(1.0f), S(1.0f);

		if (node.translation.size() == 3) {
			T = translate(node.translation);
		}
		if (node.rotation.size() == 4) {
			R = rotate(node.rotation);
		}
		if (node.scale.size() == 3) {
			S = scale(node.scale);
		}

		localTransform = T * R * S;
	}

	glm::mat4 globalTransform = parentTransform * localTransform;

	if (node.mesh > -1) {
		MeshInstance instance{};
		instance.meshIndex = node.mesh;
		instance.transform = globalTransform;

		instance.instanceCustomIndex = scene.meshes[node.mesh].firstGeometry;
		instance.instanceShaderBindingTableRecordOffset = 0;

		scene.instances.push_back(instance);
	}

	if (node.light > -1 && node.light < static_cast<int>(model.lights.size())) {
		Light& light = scene.lights[node.light];

		glm::vec4 worldDir = globalTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		light.direction = Vector3f(worldDir.x, worldDir.y, worldDir.z);

		light.position = Vector3f(globalTransform[3][0], globalTransform[3][1], globalTransform[3][2]);
	}

	if (node.camera > -1 && node.camera < static_cast<int>(model.cameras.size())) {
		CameraData& cam = scene.dynamicInfo.camera;
		cam.position = Vector3f(globalTransform[3][0], globalTransform[3][1], globalTransform[3][2]);

		glm::vec4 worldForward = globalTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		glm::vec4 worldUp = globalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

		cam.direction = Vector3f(worldForward.x, worldForward.y, worldForward.z);
		cam.up = Vector3f(worldUp.x, worldUp.y, worldUp.z);
	}

	for (int childIndex : node.children) {
		processNode(model.nodes[childIndex], globalTransform, scene);
	}
}

VkFormat SceneLoader::findTextureFormat(const TextureData& tex, bool ifSRGB) {
	uint32_t bits = tex.bits;

	switch (bits) {
	case 8:
		return ifSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	case 16:
		return ifSRGB ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_R16G16B16A16_SFLOAT;
	case 32:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		throw std::runtime_error("The texture format is not supported");
	}
}

VkFilter SceneLoader::glFilterToVk(int glFilter) {
	switch (glFilter) {
	case TINYGLTF_TEXTURE_FILTER_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		return VK_FILTER_NEAREST;

	case TINYGLTF_TEXTURE_FILTER_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		return VK_FILTER_LINEAR;

	default:
		return VK_FILTER_LINEAR;
	}
}

VkSamplerAddressMode SceneLoader::glWrapToVk(int glWrap) {
	switch (glWrap) {
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
	default:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

glm::mat4 SceneLoader::translate(const std::vector<double>& t) {
	glm::mat4 m(1.0f);
	m[3][0] = static_cast<float>(t[0]);
	m[3][1] = static_cast<float>(t[1]);
	m[3][2] = static_cast<float>(t[2]);
	return m;
}

glm::mat4 SceneLoader::scale(const std::vector<double>& s) {
	glm::mat4 m(1.0f);
	m[0][0] = static_cast<float>(s[0]);
	m[1][1] = static_cast<float>(s[1]);
	m[2][2] = static_cast<float>(s[2]);
	return m;
}

glm::mat4 SceneLoader::rotate(const std::vector<double>& r) {
	// glTF quaternion is [x, y, z, w]
	float x = static_cast<float>(r[0]);
	float y = static_cast<float>(r[1]);
	float z = static_cast<float>(r[2]);
	float w = static_cast<float>(r[3]);

	glm::mat4 m(1.0f);

	float x2 = x + x, y2 = y + y, z2 = z + z;
	float xx = x * x2, xy = x * y2, xz = x * z2;
	float yy = y * y2, yz = y * z2, zz = z * z2;
	float wx = w * x2, wy = w * y2, wz = w * z2;

	m[0][0] = 1.0f - (yy + zz);
	m[0][1] = xy + wz;
	m[0][2] = xz - wy;

	m[1][0] = xy - wz;
	m[1][1] = 1.0f - (xx + zz);
	m[1][2] = yz + wx;

	m[2][0] = xz + wy;
	m[2][1] = yz - wx;
	m[2][2] = 1.0f - (xx + yy);

	return m;
}

float SceneLoader::computeMeshLightArea(const Scene& scene, const Mesh& mesh, const Geometry& geo, const Light& light) const {
	float totalArea = 0.0f;
	for (uint32_t triIdx = 0; triIdx < geo.primitiveCount; ++triIdx) {
		uint32_t baseIdx = mesh.firstIndex + geo.firstIndex + triIdx * 3;
		uint32_t i0 = scene.indices[baseIdx + 0];
		uint32_t i1 = scene.indices[baseIdx + 1];
		uint32_t i2 = scene.indices[baseIdx + 2];

		Vector3f v0_local = scene.vertices[mesh.firstVertex + geo.firstVertex + i0];
		Vector3f v1_local = scene.vertices[mesh.firstVertex + geo.firstVertex + i1];
		Vector3f v2_local = scene.vertices[mesh.firstVertex + geo.firstVertex + i2];

		glm::vec4 p0 = light.transform * glm::vec4(v0_local.data, 1.0f);
		glm::vec4 p1 = light.transform * glm::vec4(v1_local.data, 1.0f);
		glm::vec4 p2 = light.transform * glm::vec4(v2_local.data, 1.0f);

		Vector3f v0_world(p0.x, p0.y, p0.z);
		Vector3f v1_world(p1.x, p1.y, p1.z);
		Vector3f v2_world(p2.x, p2.y, p2.z);

		Vector3f e1 = v1_world - v0_world;
		Vector3f e2 = v2_world - v0_world;

		totalArea += 0.5f * Cross(e1, e2).norm();
	}
	return totalArea;
}

float SceneLoader::computeLightPower(const Light& light) const {
	const float PI = 3.1415926;

	float luminance = 0.2126 * light.emission.x() + 0.7152 * light.emission.y() + 0.0722 * light.emission.z();

	switch (light.lightType) {
	case LIGHT_TYPE_MESH:
		return std::max(light.area * PI * luminance, 0.001f);
	case LIGHT_TYPE_DIRECTIONAL:
		return std::max(PI * luminance, 0.001f);
	case LIGHT_TYPE_POINT:
		return std::max(4.0f * PI * luminance, 0.001f);
	case LIGHT_TYPE_SPOT: {
		float solidAngle = 2.0f * PI * (1.0f - std::cos(light.outerConeAngle));
		return std::max(solidAngle * luminance, 0.001f);
	}
	default:
		return 0.001f;
	}

	
}