#ifndef VULKAN_SYNC_OBJECT
#define VULKAN_SYNC_OBJECT

#include "base/base.h"
#include "Context.h"

class Fence {
public:
	Fence(VkDevice device, bool if_singled = false);

	Fence(Fence&& other) noexcept;
	Fence& operator=(Fence&& other) noexcept;

	~Fence();
	
	void wait(uint64_t timeout = UINT64_MAX) const;
	static void wait_for_fences(const pstd::vector<Fence>& fences, bool if_waitall = true, uint64_t timeout = UINT64_MAX);
	
	void reset() const;
	static void reset_fences(const pstd::vector<Fence>& fences);

	operator VkFence() const { return _fence; }

	
private:
	VkFence _fence = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};

class Semaphore {
public:
	Semaphore(VkDevice device);

	Semaphore(Semaphore&& other) noexcept;
	Semaphore& operator=(Semaphore&& other) noexcept;

	~Semaphore();

	operator VkSemaphore() const { return _semaphore; }

	VkSemaphore get() const { return _semaphore; }

private:

	VkSemaphore _semaphore = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};

#endif // !VULKAN_SYNC_OBJECT

