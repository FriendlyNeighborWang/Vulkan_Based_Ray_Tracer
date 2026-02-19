#include "SyncObject.h"

#include "Context.h"

Fence::Fence(VkDevice device, bool if_singled):_device(device) {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = if_singled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	if (vkCreateFence(device, &fenceInfo, nullptr, &_fence) != VK_SUCCESS)
		throw std::runtime_error("Fence::Failed to create Fence");
}

Fence::Fence(Fence&& other) noexcept :_device(other._device), _fence(other._fence) {
	other._fence = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
}

Fence& Fence::operator=(Fence&& other) noexcept {
	if (this != &other) {
		if (_fence != VK_NULL_HANDLE)vkDestroyFence(_device, _fence, nullptr);
	}
	

	_fence = other._fence;
	_device = other._device;

	other._fence = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
	
	return *this;
}

Fence::~Fence() {
	if (_fence != VK_NULL_HANDLE)vkDestroyFence(_device, _fence, nullptr);
}

void Fence::wait(uint64_t timeout) const {
	vkWaitForFences(_device, 1, &_fence, VK_TRUE, timeout);
}

void Fence::wait_for_fences(const pstd::vector<Fence>& fences, bool if_waitall, uint64_t timeout) {
	if (fences.empty())return;

	pstd::vector<VkFence> vkFences;
	vkFences.resize(fences.size());

	for (uint32_t i = 0; i < fences.size(); ++i) {
		vkFences[i] = fences[i]._fence;
	}

	vkWaitForFences(fences[0]._device, vkFences.size(), vkFences.data(), if_waitall, timeout);
}

void Fence::reset() const {
	vkResetFences(_device, 1, &_fence);
}

void Fence::reset_fences(const pstd::vector<Fence>& fences) {
	if (fences.empty())return;

	pstd::vector<VkFence> vkFences;
	vkFences.resize(fences.size());

	for (uint32_t i = 0; i < fences.size(); ++i) {
		vkFences[i] = fences[i]._fence;
	}

	vkResetFences(fences[0]._device, vkFences.size(), vkFences.data());
}

Semaphore::Semaphore(VkDevice device) :_device(device) {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_semaphore) != VK_SUCCESS)
		throw std::runtime_error("Semaphore::Failed to create semaphore");
}

Semaphore::Semaphore(Semaphore&& other) noexcept : _semaphore(other._semaphore), _device(other._device) {
	other._semaphore = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
}
	
Semaphore & Semaphore::operator=(Semaphore&& other) noexcept {
	if (this != &other) {
		if (_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(_device, _semaphore, nullptr);
	}

	_semaphore = other._semaphore;
	_device = other._device;

	other._semaphore = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;

	return *this;
}

Semaphore::~Semaphore() {
	if (_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(_device, _semaphore, nullptr);
}