#ifndef PBRT_GRAPHICS_SKYBOX
#define PBRT_GRAPHICS_SKYBOX

#include "base/base.h"
#include "util/pstd.h"
#include "vk_layer/Texture.h"

class SkyBox {
public:
	SkyBox();
	SkyBox(const std::string& filePath);

	SkyBox(SkyBox&& other) noexcept;
	SkyBox& operator=(SkyBox&& other) noexcept;

	pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*> get_skybox_textures_and_samplers(Context& context);

	float get_total_power() { return totalPower; }

private:

	static float luminance(float r, float g, float b) {
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}

	pstd::vector<float> hdrData;
	pstd::vector<float> conditionalCdf;
	pstd::vector<float> marginalCdf;

	uint32_t _width, _height, _channels;

	// envTexture, conditionalCdfTexture, marginalCdfTexture; 
	pstd::vector<Texture> textures;
	// hdrSampler, cdfSampler
	pstd::vector<Sampler> samplers;

	float totalPower = 0.0f;

	bool if_use = false;
};

#endif // !PBRT_GRAPHICS_SKYBOX

