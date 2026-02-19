#ifndef VULKAN_SWAPCHAIN
#define VULKAN_SWAPCHAIN

#include "base/base.h"
#include "util/pstd.h"


struct GLFWwindow;
class Context;

class SwapChain {
public:
	SwapChain(Context& context, GLFWwindow* window);
	~SwapChain();

	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	void recreate(GLFWwindow* window);

	VkResult acquire_next_image(VkSemaphore semaphore, VkFence fence, uint32_t* imageIndex, uint32_t timeout = UINT64_MAX);

	static bool check_device_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);

	void image_layout_transtion(uint32_t idx, CommandBuffer& cmdBuffer, VkImageLayout targetLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

	VkResult present(uint32_t imageIndex, const pstd::vector<VkSemaphore>& waitSemaphores);

	VkSwapchainKHR get() const { return swapChain; }
	VkFormat getImageFormat() const { return _format; }
	VkExtent2D getExtent() const { return _extent; }
	VkImage getImage(uint32_t idx) const { return _images[idx]; }
	VkImageView getImageView(uint32_t idx) const { return _views[idx]; }
	VkImageLayout getImageLayout(uint32_t idx) const { return _layouts[idx]; }
	uint32_t imageCount()const { return _images.size(); }
	

private:
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		pstd::vector<VkSurfaceFormatKHR> formats;
		pstd::vector<VkPresentModeKHR> presentModes;
	};

	static SwapChainSupportDetails get_swapchain_support_details(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const pstd::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const pstd::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	pstd::vector<VkImage> _images;
	pstd::vector<VkImageView> _views;
	pstd::vector<VkImageLayout> _layouts;
	VkFormat _format;
	VkExtent2D _extent;

	Context& _context;
};

#endif // !VULKAN_SWAPCHAIN
