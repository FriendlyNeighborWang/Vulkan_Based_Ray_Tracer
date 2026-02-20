#ifndef VULKAN_COMMAND_POOL
#define VULKAN_COMMAND_POOL

#include "base/base.h"
#include "util/pstd.h"
#include "util/Threadsafe_queue.h"

// TODO:: Multi-threading

class CommandBuffer {
	friend class CommandPool;
public:
	CommandBuffer(CommandBuffer&& other) noexcept;

	void begin(bool one_time = false);

	void end_and_submit(VkQueue queue, bool if_waitIdle = false);
	void end_and_submit(VkQueue queue, const pstd::vector<VkSemaphore>& waitSemaphores, const pstd::vector<VkPipelineStageFlags>& waitStages, const pstd::vector<VkSemaphore>& signalSemaphores, VkFence fence = VK_NULL_HANDLE);

	operator const VkCommandBuffer& () const { return cmdBuffer; }
	
	~CommandBuffer();

private:
	CommandBuffer(VkCommandBuffer command_buffer, CommandPool& commandPool);

	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
	CommandPool& cmdPool;
};

class CommandPool {
	friend class CommandBuffer;
public:
	CommandPool(Context& context);

	void init(uint32_t pre_alloc_size = 1);

	CommandBuffer get_command_buffer();

	uint32_t allocate_command_buffer(uint32_t num = 1);

	uint32_t available_num() { return cmdBuffer_queue.size(); }

	~CommandPool();

private:

	pstd::Threadsafe_queue<VkCommandBuffer> cmdBuffer_queue;

	uint32_t total_allocated = 0;

	VkCommandPool cmdPool = VK_NULL_HANDLE;
	Context& _context;
};


#endif // !VULKAN_COMMAND_POOL
