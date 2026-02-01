#ifndef PBRT_UTIL_SCENE_LOADER_H
#define PBRT_UTIL_SCENE_LOADER_H

#include "base/base.h"
#include "util/memory.h"
#include "util/Vec.h"
#include "Scene.h"

#include "deps/tiny_gltf/tiny_gltf.h"

class SceneLoader {
	using Model = tinygltf::Model;
	using Accessor = tinygltf::Accessor;
	using BufferView = tinygltf::BufferView;
	using Node = tinygltf::Node;

public:
	static SceneLoader& Get() {
		static SceneLoader instance;
		return instance;
	}

	Scene LoadScene(const std::string& filePath) {
		Model model;
		tinygltf::TinyGLTF loader;

		std::string err;
		std::string warn;

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

		loadMaterial(model, outScene);

		loadMeshes(model, outScene);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		glm::mat4 rootTransform(1.0f);
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			processNode(model, model.nodes[scene.nodes[i]], rootTransform, outScene);
		}

		LOG_STREAM("SceneLoader") << " Materials: " << outScene.materials.size() << std::endl;
		LOG_STREAM("SceneLoader") << " Meshes (BLAS): " << outScene.materials.size() << std::endl;
		LOG_STREAM("SceneLoader") << " Instances (TLAS): " << outScene.instances.size() << std::endl;

		return outScene;
	}


private:
	SceneLoader() = default;

	void loadMaterial(const Model& model, Scene& scene) {
		if (model.materials.empty()) return;

		scene.materials.reserve(model.materials.size());
		
		for (const auto& mat : model.materials) {
			Material material{};

			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				const auto& color = mat.values.at("baseColorFactor").ColorFactor();
				material.baseColor = Vector4f((float)color[0], (float)color[1], (float)color[2], (float)color[3]);
			}
			if (mat.values.find("metallicFactor") != mat.values.end()) {
				material.metallic = (float)mat.values.at("metallicFactor").Factor();
			}
			if (mat.values.find("roughnessFactor") != mat.values.end()) {
				material.roughness = (float)mat.values.at("roughnessFactor").Factor();
			}


			scene.materials.push_back(material);
		}
	}

	void loadMeshes(const Model& model, Scene& scene) {
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

				size_t vertexCount = 0;

				if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
					const Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
					const BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					positionBuffer = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
					const Accessor& accessor = model.accessors[primitive.attributes.at("NORMAL")];
					const BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					normalBuffer = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
				}

				for (size_t v = 0; v < vertexCount; ++v) {
					Vector3f pos = { positionBuffer[v * 3 + 0], positionBuffer[v * 3 + 1], positionBuffer[v * 3 + 2] };
					Normal norm = normalBuffer ? Normal(normalBuffer[v * 3 + 0], normalBuffer[v * 3 + 1], normalBuffer[v * 3 + 2]) : Normal{ 0.0,0.0,0.0 };


					scene.vertices.push_back(pos);
					scene.normals.push_back(norm);
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

	void processNode(const Model& model, const Node& node, const glm::mat4& parentTransform, Scene& scene) {
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

		for (int childIndex : node.children) {
			processNode(model, model.nodes[childIndex], globalTransform, scene);
		}
	}


	glm::mat4 translate(const std::vector<double>& t) {
		glm::mat4 m(1.0f);
		m[3][0] = static_cast<float>(t[0]);
		m[3][1] = static_cast<float>(t[1]);
		m[3][2] = static_cast<float>(t[2]);
		return m;
	}

	glm::mat4 scale(const std::vector<double>& s) {
		glm::mat4 m(1.0f);
		m[0][0] = static_cast<float>(s[0]);
		m[1][1] = static_cast<float>(s[1]);
		m[2][2] = static_cast<float>(s[2]);
		return m;
	}

	glm::mat4 rotate(const std::vector<double>& r) {
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

};

	





#endif // !PBRT_UTIL_SCENE_LOADER_H

