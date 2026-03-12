#ifndef PBRT_GRAPHICS_FRAME_CONTEXT_H
#define PBRT_GRAPHICS_FRAME_CONTEXT_H

#include "base/base.h"
#include "util/pstd.h"
#include "vk_layer/CommandPool.h"

class FrameContext {
public:
	// CommandBufer
	CommandBuffer cmdBuffer;

	// Descriptor Set
	std::unordered_map<std::string, std::vector<VkDescriptorSet>> descripotrSets;

	// Other Resources
	std::unordered_map<std::string, Image*> images;
	std::unordered_map<std::string, Buffer*> buffers;
};

#endif // !PBRT_GRAPHICS_FRAME_CONTEXT_H
