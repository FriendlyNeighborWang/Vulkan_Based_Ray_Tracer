#include "CommandPool.h"

#include "vk_layer/Context.h"

CommandBuffer::CommandBuffer(VkCommandBuffer command_buffer, CommandPool& commandPool):cmdBuffer(command_buffer), cmdPool(commandPool){}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept : cmdBuffer(other.cmdBuffer), cmdPool(other.cmdPool) {
	other.cmdBuffer = VK_NULL_HANDLE;
}

CommandBuffer::~CommandBuffer() {
	if(cmdBuffer!=VK_NULL_HANDLE)
		cmdPool.cmdBuffer_queue.push(cmdBuffer);
}

void CommandBuffer::begin(bool one_time) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (one_time)
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(cmdBuffer, &beginInfo))
		throw std::runtime_error("CommandPool:: Failed to begin command buffer");
}

void CommandBuffer::end_and_submit(VkQueue queue, bool if_waitIdle) {
	if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		throw std::runtime_error("CommandBuffer:: Failed to end commandbuffer");

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		throw std::runtime_error("CommandPool::Failed to submit");

	if (if_waitIdle)
		vkQueueWaitIdle(queue);
}

void CommandBuffer::end_and_submit(VkQueue queue, const pstd::vector<VkSemaphore>& waitSemaphores, VkPipelineStageFlags waitStages, const pstd::vector<VkSemaphore>& signalSemaphores, VkFence fence) {
	if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
		throw std::runtime_error("CommandBuffer:: Failed to end commandbuffer");

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	submitInfo.pWaitDstStageMask = &waitStages;
	
	
	if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
		throw std::runtime_error("CommandPool::Failed to submit");
}




CommandPool::CommandPool(Context& context) :_context(context) {}

void CommandPool::init(uint32_t pre_alloc_size) {
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = _context.gc_queue().index;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(_context, &createInfo, nullptr, &cmdPool) != VK_SUCCESS)
		throw std::runtime_error("CommandPool:: Failed to create command pool");

	if (pre_alloc_size == 0)return;

	VkCommandBufferAllocateInfo cmdAllocInfo{};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = cmdPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = pre_alloc_size;

	pstd::vector<VkCommandBuffer> tmp_cmd_buffers(pre_alloc_size);

	if (vkAllocateCommandBuffers(_context, &cmdAllocInfo, tmp_cmd_buffers.data()) != VK_SUCCESS)
		throw std::runtime_error("CommandPool:: Failed to allocate command buffer");

	for (auto& cmdBuffer : tmp_cmd_buffers)
		cmdBuffer_queue.push(cmdBuffer);

	total_allocated = pre_alloc_size;
}


CommandBuffer CommandPool::get_command_buffer() {
	if (!total_allocated)allocate_command_buffer();
	return CommandBuffer(cmdBuffer_queue.pop(), *this);
}

uint32_t CommandPool::allocate_command_buffer(uint32_t num) {
	pstd::vector<VkCommandBuffer> tmp_list(num);
	
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = num;
	allocInfo.commandPool = cmdPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(_context, &allocInfo, tmp_list.data()) != VK_SUCCESS)
		throw std::runtime_error("CommandPool:: Failed to allocate command buffers");

	for (auto cmdBuffer : tmp_list) {
		cmdBuffer_queue.push(cmdBuffer);
	}

	total_allocated += num;

	return total_allocated;
}


CommandPool::~CommandPool() {
	vkDeviceWaitIdle(_context);

	if (cmdPool != VK_NULL_HANDLE)
		vkDestroyCommandPool(_context, cmdPool, nullptr);

	LOG_STREAM("CommandPool") << "CommandPool has been deconstructed" << std::endl;
}



