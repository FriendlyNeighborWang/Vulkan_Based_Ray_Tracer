#include "SkyBox.h"

#include "Scene.h"
#include "vk_layer/Context.h"
#include "vk_layer/VkMemoryAllocator.h"

#include "stb_image.h"

SkyBox::SkyBox(): if_use(false){}


SkyBox::SkyBox(const std::string& filePath): if_use(true) {
	const float PI = 3.1415926535f;

	// Load Data
	int w, h, channels;

	float* raw = stbi_loadf(filePath.c_str(), &w, &h, &channels, 0);
	if (!raw) {
		throw std::runtime_error("SkyBox::Failed to load SkyBox src image");
	}

	_width = w; _height = h; _channels = channels;

	hdrData.resize(w * h * 4);

	for (uint32_t i = 0; i < w * h; ++i) {
		hdrData[i * 4 + 0] = raw[i * channels + 0];
		hdrData[i * 4 + 1] = raw[i * channels + 1];
		hdrData[i * 4 + 2] = raw[i * channels + 2];
		hdrData[i * 4 + 3] = (channels >= 4) ? raw[i * channels + 3] : 1.0f;
	}

	channels = 4;

	stbi_image_free(raw);


	// Compute Weight
	pstd::vector<float> weightMap(w * h);
	pstd::vector<float> rowWeightMap(h, 0.0f);

	for (uint32_t y = 0; y < h; ++y) {
		float theta = PI * (static_cast<float>(y) + 0.5f) / static_cast<float>(h);
		float sinTheta = std::sin(theta);

		for (uint32_t x = 0; x < w; ++x) {
			uint32_t idx = x + y * w;
			float r = hdrData[idx * channels + 0];
			float g = hdrData[idx * channels + 1];
			float b = hdrData[idx * channels + 2];

			float lum = luminance(r, g, b);

			weightMap[idx] = lum * sinTheta;
		}

		for (uint32_t x = 0; x < w; ++x) {
			rowWeightMap[y] += weightMap[y * w + x];
		}
	}

	// Conditional CDF

	conditionalCdf.resize(w * h);

	for (uint32_t y = 0; y < h; ++y) {
		float rowSum = rowWeightMap[y];
		float culmulativeWeight = 0.0f;
		for (uint32_t x = 0; x < w; ++x) {
			if (rowSum <= 0.0f) {
				conditionalCdf[y * w + x] = static_cast<float>(x + 1) / static_cast<float>(w);
			}
			else {

				culmulativeWeight += weightMap[y * w + x];
				conditionalCdf[y * w + x] = culmulativeWeight / rowSum;
			}
		}
		conditionalCdf[y * w + (w - 1)] = 1.0f;
	}


	// Marginal CDF
	float totalWeight = 0.0f;

	for (uint32_t y = 0; y < h; ++y) {
		totalWeight += rowWeightMap[y];
	}
	totalPower = totalWeight;
	marginalCdf.resize(h);


	float cumulativeWeight = 0.0f;
	for (uint32_t y = 0; y < h; ++y) {
		if (totalWeight <= 0.0f) {
			marginalCdf[y] = static_cast<float>(y + 1) / static_cast<float>(h);
		}
		else {
			cumulativeWeight += rowWeightMap[y];
			marginalCdf[y] = cumulativeWeight / totalWeight;
		}
	}
	marginalCdf[h - 1] = 1.0f;


}

pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*> SkyBox::get_skybox_textures_and_samplers(Context& context) {
	using ReturnType = pstd::tuple<pstd::vector<Texture>*, pstd::vector<Sampler>*>;
	if (!textures.empty()) return ReturnType(&textures, &samplers);

	if (!if_use) {
		samplers.emplace_back(context, SamplerData{});
		pstd::vector<TextureData> skyboxTextureData{ Scene::placeholderTextureData() };
		textures = context.memAllocator().create_textures(skyboxTextureData, samplers);
		return ReturnType(&textures, &samplers);
	}


	SamplerData hdrSamplerData;
	SamplerData cdfSamplerData;
	TextureData envTextureData;
	TextureData conditionalCdfTextureData;
	TextureData marginalCdfTextureData;

	hdrSamplerData.minFilter = VK_FILTER_LINEAR;
	hdrSamplerData.magFilter = VK_FILTER_LINEAR;
	hdrSamplerData.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	hdrSamplerData.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplers.push_back(Sampler(context, hdrSamplerData));

	cdfSamplerData.minFilter = VK_FILTER_NEAREST;
	cdfSamplerData.magFilter = VK_FILTER_NEAREST;
	cdfSamplerData.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cdfSamplerData.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplers.push_back(Sampler(context, cdfSamplerData));

	envTextureData.width = _width;
	envTextureData.height = _height;
	envTextureData.channels = _channels;
	envTextureData.data = reinterpret_cast<unsigned char*>(hdrData.data());
	envTextureData.bits = 32;
	envTextureData.size = static_cast<VkDeviceSize>(hdrData.size() * sizeof(float));
	envTextureData.samplerIdx = 0;
	envTextureData.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	conditionalCdfTextureData.width = _width;
	conditionalCdfTextureData.height = _height;
	conditionalCdfTextureData.channels = 1;
	conditionalCdfTextureData.data = reinterpret_cast<unsigned char*>(conditionalCdf.data());
	conditionalCdfTextureData.bits = 32;
	conditionalCdfTextureData.size = static_cast<VkDeviceSize>(conditionalCdf.size() * sizeof(float));
	conditionalCdfTextureData.samplerIdx = 1;
	conditionalCdfTextureData.format = VK_FORMAT_R32_SFLOAT;

	marginalCdfTextureData.width = _height;
	marginalCdfTextureData.height = 1;
	marginalCdfTextureData.channels = 1;
	marginalCdfTextureData.data = reinterpret_cast<unsigned char*> (marginalCdf.data());
	marginalCdfTextureData.bits = 32;
	marginalCdfTextureData.size = static_cast<VkDeviceSize>(marginalCdf.size() * sizeof(float));
	marginalCdfTextureData.samplerIdx = 1;
	marginalCdfTextureData.format = VK_FORMAT_R32_SFLOAT;

	textures = context.memAllocator().create_textures(
		{ envTextureData, conditionalCdfTextureData, marginalCdfTextureData },
		samplers
	);

	return ReturnType(&textures, &samplers);
}




