#ifndef PBRT_UTIL_SCENE_LOADER_H
#define PBRT_UTIL_SCENE_LOADER_H

#include "base/base.h"
#include "util/memory.h"
#include "util/Vec.h"
#include "Mesh.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class SceneLoader {
public:
	static SceneLoader& Get() {
		static SceneLoader instance;
		return instance;
	}

	SceneLoader(const SceneLoader&) = delete;
	SceneLoader& operator=(const SceneLoader&) = delete;

	bool LoadScene(const std::string& filepath) {

		scene = importer.ReadFile(filepath,
			aiProcess_Triangulate |
			aiProcess_GenNormals |
			aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType
		);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
			return false;
		}

		LOG_STREAM("scene_loader") << "Scene loaded successfully!" << std::endl;
		LOG_STREAM("scene_loader") << "Meshes count: " << scene->mNumMeshes << std::endl;
		LOG_STREAM("scene_loader") << "Material count: " << scene->mNumMaterials << std::endl;

		return true;
	}

	pstd::vector<Mesh> getMesh() {
		assert(scene && "SceneLoader::getMesh: model has not been loaded");

		pstd::vector<Mesh> meshes;
		meshes.reserve(scene->mNumMeshes);

		// Load Scene
		for (size_t i = 0; i < scene->mNumMeshes; ++i) {
			aiMesh* a_mesh = scene->mMeshes[i];
			Mesh mesh;

			LOG_STREAM("scene_loader") << "\n=== Processing Mesh: " << a_mesh->mName.C_Str() << " ===" << std::endl;

			// Vertex Loading
			getVertices(a_mesh, mesh);

			LOG_STREAM("scene_loader") << "Vertices: " << mesh.vertices.size() << std::endl;

			// Index Loading
			getIndices(a_mesh, mesh);

			LOG_STREAM("scene_loader") << "Indices: " << mesh.indices.size() << std::endl;

			// Normal Loading
			if (a_mesh->HasNormals()) {
				getNormals(a_mesh, mesh);
			}
			LOG_STREAM("scene_loader") << "Normals: " << mesh.normals.size() << std::endl;

			// Texture Coordinate Loading
			if (a_mesh->HasTextureCoords(0)) {
				getTexCoords(a_mesh, mesh);
			}
			meshes.push_back(std::move(mesh));
		}



		return meshes;
	}

	pstd::vector<MeshInstance> getInstance() {
		pstd::vector<MeshInstance> instances;
		getInstance_recursion(scene->mRootNode, glm::mat4(1.0f), instances);
		return instances;
	}


private:
	
	SceneLoader() = default;
	
	void getVertices(aiMesh* a_mesh, Mesh& mesh) {
		mesh.vertices.reserve(a_mesh->mNumVertices);
		for (size_t i = 0; i < a_mesh->mNumVertices; ++i) {
			auto pos = &a_mesh->mVertices[i];
			mesh.vertices.emplace_back(pos->x, pos->y, pos->z);
		}
	}

	void getIndices(aiMesh* a_mesh, Mesh& mesh) {
		mesh.indices.reserve(a_mesh->mNumFaces * 3);
		for (size_t i = 0; i < a_mesh->mNumFaces; ++i) {
			auto face = &a_mesh->mFaces[i];
			assert(face->mNumIndices == 3 && "SceneLoader::getIndices: Face is not a triangle");
			for (size_t j = 0; j < face->mNumIndices; ++j) {
				mesh.indices.push_back(face->mIndices[j]);
			}
		}
	}

	void getNormals(aiMesh* a_mesh, Mesh& mesh) {
		mesh.normals.reserve(a_mesh->mNumVertices);
		for (size_t i = 0; i < a_mesh->mNumVertices; ++i) {
			auto normal = &a_mesh->mNormals[i];
			mesh.normals.emplace_back(normal->x, normal->y, normal->z);
		}
	}

	void getTexCoords(aiMesh* a_mesh, Mesh& mesh) {
		mesh.texCoords.reserve(a_mesh->mNumVertices);
		for (size_t i = 0; i < a_mesh->mNumVertices; ++i) {
			auto uv = &a_mesh->mTextureCoords[0][i];
			mesh.texCoords.emplace_back(uv->x, uv->y);
		}
	}

	void getInstance_recursion(aiNode* node, const glm::mat4& parentTransform, pstd::vector<MeshInstance>& instances) {
		glm::mat4 localTransform = aiMatrix_to_glm(node->mTransformation);
		glm::mat4 worldTransform = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
			MeshInstance inst{};
			inst.meshIndex = node->mMeshes[i];
			inst.transform = worldTransform;
			instances.push_back(inst);
		}
		
		for (uint32_t i = 0; i < node->mNumChildren; ++i) {
			getInstance_recursion(node->mChildren[i], worldTransform, instances);
		}
	}

	glm::mat4 aiMatrix_to_glm(const aiMatrix4x4& m) {
		return glm::mat4(
			m.a1, m.b1, m.c1, m.d1, 
			m.a2, m.b2, m.c2, m.d2, 
			m.a3, m.b3, m.c3, m.d3, 
			m.a4, m.b4, m.c4, m.d4
		);
	}

	Assimp::Importer importer;
	const aiScene* scene = nullptr;
};

#endif // !PBRT_UTIL_SCENE_LOADER_H

