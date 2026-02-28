#ifndef PBRT_GRAPHICS_SKYBOX
#define PBRT_GRAPHICS_SKYBOX

#include "base/base.h"
#include "util/pstd.h"
#include "vk_layer/Texture.h"

class SkyBox {
public:
	SkyBox(Context& context);

	SkyBox(Context& context, const std::string& filePath);

	const float get_total_power() const { return totalPower; }
	const pstd::vector<Texture>& get_skybox_textures() const { return textures; }

private:

	static float luminance(float r, float g, float b) {
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}

	pstd::vector<float> hdrData;
	pstd::vector<float> conditionalCdf;
	pstd::vector<float> marginalCdf;
	float totalPower = 0.0f;

	// envTexture, conditionalCdfTexture, marginalCdfTexture; 
	pstd::vector<Texture> textures;
	// hdrSampler, cdfSampler
	pstd::vector<Sampler> samplers;

};

#endif // !PBRT_GRAPHICS_SKYBOX

