#ifndef VULKAN_CONTEXT
#define VULKAN_CONTEXT

#include "base/base.h"
#include "util/pstd.h"

#include <functional>
#include <memory>

struct GLFWwindow;

class Context {
public:
	Context();

	// Instance
	static void add_instance_extension(const char* extension);
	static void enable_validation_layers();
	void create_instance();

	// Surface
	void create_surface(GLFWwindow* window);

	// Physical Device
	static void add_device_extension(const char* extension);

	template <typename Validator>
	static void register_device_feature(VkPhysicalDeviceFeatures2& features, Validator validator) {
		device_features = features;
		features_validator = validator;
	}

	void pick_physical_device();

	// Logical Device
	struct Queue {
		VkQueue queue = VK_NULL_HANDLE;
		uint32_t index = 0;
		operator const VkQueue& () const;
	};

	void create_device();

	// Initialize
	void init();

	// Member Access
	operator const VkInstance& () const;
	operator const VkPhysicalDevice& () const;
	operator const VkDevice& () const;
	
	ShaderManager& shaderManager();
	VkMemoryAllocator& memAllocator();
	CommandPool& cmdPool();
	DescriptorManager& descriptorManager();
	ASManager& asManager();
	RTPipeline& rtPipeline();

	Queue& gc_queue();
	Queue& present_queue();
	VkSurfaceKHR& device_surface();

	~Context();

private:
	// Instance helpers
	static bool checkInstanceExtensionSupport();
	static bool checkValidationLayerSupport();
	static VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();

	// Physical Device helpers
	struct QueueFamilyIndices {
		pstd::optional<uint32_t> graphicsAndComputeFamily;
		pstd::optional<uint32_t> presentFamily;

		bool isComplete() const;
	};

	static bool checkDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	static bool checkDeviceFeatureSupport(VkPhysicalDevice device);

	// Queues
	Queue graphicsAndComputeQueue;
	Queue presentQueue;

	// Static members
	inline static pstd::vector<const char*> instance_extensions;
	inline static pstd::vector<const char*> device_extensions;
	inline static VkPhysicalDeviceFeatures2 device_features;
	inline static std::function<bool()> features_validator;
	inline static bool if_enable_validation_layers = false;
	inline static pstd::vector<const char*> validation_layers;

	// Other components
	std::unique_ptr<ShaderManager> _shaderManager;
	std::unique_ptr<VkMemoryAllocator> _memAllocator;
	std::unique_ptr<CommandPool> _cmdPool;
	std::unique_ptr<DescriptorManager> _descriptorManager;
	std::unique_ptr<ASManager> _asManager;
	std::unique_ptr<RTPipeline> _rtPipeline;


	// Instance members
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
};

#endif // !VULKAN_CONTEXT
