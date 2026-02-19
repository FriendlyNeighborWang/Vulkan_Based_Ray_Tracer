#include "VkMemoryAllocator.h"

#include "vk_layer/Buffer.h"
#include "vk_layer/Context.h"
#include "vk_layer/Texture.h"
#include "vk_layer/CommandPool.h"

#include "graphics/Scene.h"

VkMemoryAllocator::VkMemoryAllocator(Context& context):_context(context){}

VkMemoryAllocator::~VkMemoryAllocator() {
	LOG_STREAM("VkMemoryAllocator") << "VkMemoryAllocator has been deconstructed" << std::endl;
}

void VkMemoryAllocator::init(){}

Image VkMemoryAllocator::create_image(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;
	
	VkImage img;
	if (vkCreateImage(_context, &imageInfo, nullptr, &img) != VK_SUCCESS)
		throw std::runtime_error("VkMemoryAllocator::Failed to create Image");
	
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(_context, img, &memReq);

	VkDeviceMemory memory = allocate_memory(memReq, properties);
	if (vkBindImageMemory(_context, img, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("VkMemoryAllocator::Failed to bind memory to Image");

	
	VkImageAspectFlags aspect = getAspectFlags(format);
	VkImageView imgView = VK_NULL_HANDLE;
	if (tiling != VK_IMAGE_TILING_LINEAR)
		imgView = Image::create_imageview(_context, img, format, aspect);

	Image image(_context, memory, img, imgView, extent, tiling, VK_IMAGE_LAYOUT_UNDEFINED, format, memReq.size);
	

	return image;
}

Buffer VkMemoryAllocator::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buf;
	if (vkCreateBuffer(_context, &bufferInfo, nullptr, &buf) != VK_SUCCESS)
		throw std::runtime_error("VkMemoryAllocator::Failed to create Buffer");

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(_context, buf, &memReq);

	VkDeviceMemory memory = allocate_memory(memReq, properties);

	if (vkBindBufferMemory(_context, buf, memory, 0) != VK_SUCCESS)
		throw std::runtime_error("VkMemoryAllocator::Failed to bind memory to Buffer");

	Buffer buffer(_context, memory, buf, size);

	return buffer;
}

void VkMemoryAllocator::copy_to_buffer(const void* data, VkDeviceSize size, Buffer& dstBuffer, bool keep_mapping) const{
	dstBuffer.map_memory();
	memcpy(dstBuffer.map_address, data, size);
	if (keep_mapping) return;
	dstBuffer.unmap_memory();
}

void VkMemoryAllocator::copy_to_buffer(CommandBuffer& cmdBuffer, Buffer& srcBuffer, Buffer& dstBuffer) const{
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.size = srcBuffer.size;
	copyRegion.dstOffset = 0;
	
	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void VkMemoryAllocator::copy_to_buffer(CommandBuffer& cmdBuffer, Image& srcImage, Buffer& dstBuffer) const {
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageExtent = { srcImage.extent.width, srcImage.extent.height, 1 };
	region.imageOffset = { 0,0,0 };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;

	vkCmdCopyImageToBuffer(
		cmdBuffer,
		srcImage,
		srcImage.layout,
		dstBuffer,
		1, &region
	);
}


void VkMemoryAllocator::copy_to_buffer_directly(const void* data, Buffer& dstBuffer) const{
	Buffer stagingBuffer = create_buffer(
		dstBuffer.size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	copy_to_buffer(data, dstBuffer.size, stagingBuffer);

	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);

	copy_to_buffer(cmdBuffer, stagingBuffer, dstBuffer);

	cmdBuffer.end_and_submit(_context.gc_queue(), true);
	
}

void VkMemoryAllocator::unmap_buffer(Buffer& buffer) const{
	buffer.unmap_memory();
}


void VkMemoryAllocator::copy_to_image(CommandBuffer& cmdBuffer, Buffer& buffer, Image& image) const{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = { image.extent.width,image.extent.height, 1 };

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	vkCmdCopyBufferToImage(
		cmdBuffer,
		buffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);
}

void VkMemoryAllocator::copy_to_image(CommandBuffer& cmdBuffer, Image& srcImage, Image& dstImage) const {
	VkImageCopy region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;

	region.srcOffset = { 0,0,0 };
	region.dstSubresource = region.srcSubresource;
	region.dstOffset = { 0,0,0 };
	region.extent = { dstImage.extent.width, dstImage.extent.height, 1 };

	vkCmdCopyImage(
		cmdBuffer,
		srcImage, srcImage.layout,
		dstImage, dstImage.layout,
		1, &region
	);
}




void VkMemoryAllocator::blit_image(CommandBuffer& cmdBuffer, VkImage srcImage, VkImageLayout srcLayout, VkExtent2D srcExtent, VkImage dstImage, VkImageLayout dstLayout, VkExtent2D dstExtent, VkFilter filter) const {
	VkImageBlit blitRegion{};

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcOffsets[0] = { 0,0,0 };	
	blitRegion.srcOffsets[1] = {
		static_cast<int32_t>(srcExtent.width),
		static_cast<int32_t>(srcExtent.height),
		1
	};
	blitRegion.dstSubresource = blitRegion.srcSubresource;
	blitRegion.dstOffsets[0] = { 0,0,0 };
	blitRegion.dstOffsets[1] = {
		static_cast<int32_t>(dstExtent.width),
		static_cast<int32_t>(dstExtent.height),
		1
	};

	vkCmdBlitImage(
		cmdBuffer,
		srcImage, srcLayout,
		dstImage, dstLayout,
		1, &blitRegion,
		filter
	);
}

pstd::vector<Texture> VkMemoryAllocator::create_textures(const pstd::vector<TextureData>& infos, pstd::vector<Sampler>& samplers) {
	pstd::vector<Texture> textures;
	textures.reserve(infos.size());

	pstd::vector<Buffer> stagingBuffers;
	stagingBuffers.reserve(infos.size());

	CommandBuffer cmdBuffer = _context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);

	for (const auto& info : infos) {
		Buffer stagingBuffer = create_buffer(
			info.size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		Image image = create_image(
			VkExtent2D{ info.width, info.height },
			info.format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		image.transition_layout(
			_context, cmdBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		copy_to_buffer(info.data, info.size, stagingBuffer);

		copy_to_image(cmdBuffer, stagingBuffer, image);

		image.transition_layout(
			_context, cmdBuffer,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, 
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
		);

		stagingBuffers.push_back(std::move(stagingBuffer));
		textures.push_back(Texture(_context, std::move(image), samplers[info.samplerIdx]));
	}
	cmdBuffer.end_and_submit(_context.gc_queue(), true);

	return textures;
}


void VkMemoryAllocator::memory_barrier(CommandBuffer& cmdBuffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
	VkMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;

	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStage, dstStage,
		0,
		1, &barrier,
		0, nullptr,
		0, nullptr
	);
}



VkDeviceMemory VkMemoryAllocator::allocate_memory(VkMemoryRequirements memReq, VkMemoryPropertyFlags properties) const{
	VkMemoryAllocateFlagsInfo flagsInfo{};
	flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
	allocInfo.pNext = &flagsInfo;

	VkDeviceMemory memory;

	if (vkAllocateMemory(_context, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("VkMemoryAllocator::Failed to allocate Device Memory");
	
	return memory;
}

uint32_t VkMemoryAllocator::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_context, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if (typeFilter & (1 << i) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	throw std::runtime_error("VkMemoryAllocator::Failed to find suitable memory type");
}

VkImageAspectFlags VkMemoryAllocator::getAspectFlags(VkFormat format) const{
	switch (format) {
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
		return VK_IMAGE_ASPECT_DEPTH_BIT;

	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

