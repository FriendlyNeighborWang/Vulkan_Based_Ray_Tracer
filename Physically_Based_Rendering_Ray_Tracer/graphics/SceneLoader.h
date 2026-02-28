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

	Scene LoadScene(const std::string& filePath);


private:
	SceneLoader() = default;

	void loadTexture(Scene& scene);

	void loadMaterial(Scene& scene);

	void loadMeshes(Scene& scene);

	void loadPunctualLights(Scene& scene);

	void loadMeshLights(Scene& scene);

	void loadCamera(Scene& scene);

	void processNode(const Node& node, const glm::mat4& parentTransform, Scene& scene);



	static VkFormat findTextureFormat(const TextureData& tex, bool ifSRGB);

	static VkFilter glFilterToVk(int glFilter);

	static VkSamplerAddressMode glWrapToVk(int glWrap);


	static glm::mat4 translate(const std::vector<double>& t);

	glm::mat4 scale(const std::vector<double>& s);

	glm::mat4 rotate(const std::vector<double>& r);

	float computeMeshLightArea(const Scene& scene, const Mesh& mesh, const Geometry& geo, const Light& light) const;

	float computeLightPower(const Light& light) const;

	
private:
	Model model;
	tinygltf::TinyGLTF loader;
};

	



#endif // !PBRT_UTIL_SCENE_LOADER_H

