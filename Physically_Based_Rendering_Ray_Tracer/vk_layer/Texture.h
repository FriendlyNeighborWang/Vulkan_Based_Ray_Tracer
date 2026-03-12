#ifndef VULKAN_TEXTURE
#define VULKAN_TEXTURE

#include "base/base.h"
#include "Image.h"

class Sampler {
public:
	Sampler() = default;
	Sampler(Context& context, const SamplerData& info);

	Sampler(Sampler&& other) noexcept;
	Sampler& operator=(Sampler&& other) noexcept;

	~Sampler();

	operator VkSampler() const { return _sampler; }
private:
	VkSampler _sampler = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
};

class Texture {
	friend class VkMemoryAllocator;
public:
	Texture() = default;

	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	~Texture() = default;

	const Image& image() const;
	const Sampler& sampler() const;

private:
	Texture(Context& context, Image&& image, Sampler& sampler);

	Image _image;
	Sampler* _sampler = nullptr;
	VkDevice _device = VK_NULL_HANDLE;
};


#endif // !VULKAN_TEXTURE

