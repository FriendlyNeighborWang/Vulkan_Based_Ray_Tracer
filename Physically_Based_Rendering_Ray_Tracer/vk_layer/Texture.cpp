#include "Texture.h"

#include "Context.h"
#include "graphics/Scene.h"


Sampler::Sampler(Context& context, const SamplerData& info):_device(context) {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = info.magFilter;
	samplerInfo.minFilter = info.minFilter;
	
	samplerInfo.addressModeU = info.addressModeU;
	samplerInfo.addressModeV = info.addressModeV;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	if (vkCreateSampler(_device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS)
		throw std::runtime_error("Sampler::Failed to create texture sampler");
}

Sampler::Sampler(Sampler&& other) noexcept :_device(other._device), _sampler(other._sampler) {
	other._sampler = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept {
	if (&other != this) {
		if (_sampler != VK_NULL_HANDLE) vkDestroySampler(_device, _sampler, nullptr);
	}

	_sampler = other._sampler;
	_device = other._device;

	other._sampler = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
	
	return *this;
}

Sampler::~Sampler() {
	if (_sampler != VK_NULL_HANDLE)
		vkDestroySampler(_device, _sampler, nullptr);
}


Texture::Texture(Context& context, Image&& image, Sampler& sampler):_device(context),_image(std::move(image)), _sampler(&sampler) {}

Texture::Texture(Texture&& other) noexcept :_device(other._device), _image(std::move(other._image)), _sampler(other._sampler) {
	other._sampler = nullptr;
	other._device = VK_NULL_HANDLE;
}

Texture& Texture::operator=(Texture&& other) noexcept {
	if (&other != this) {
		_device = other._device;
		_image = std::move(other._image);
		_sampler = other._sampler;

		other._sampler = nullptr;
		other._device = VK_NULL_HANDLE;
	}
	return *this;
}

const Image& Texture::image() const { return _image; }
const Sampler& Texture::sampler() const { return *_sampler; }