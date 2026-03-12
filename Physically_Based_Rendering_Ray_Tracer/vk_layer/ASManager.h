#ifndef VULKAN_ACCELERATION_STRUCTURE_MANAGER
#define VULKAN_ACCELERATION_STRUCTURE_MANAGER

#include "base/base.h"
#include "util/pstd.h"
#include "Buffer.h"



class BLAS {
	friend class TLAS;
public:
	BLAS(){}
	BLAS(BLAS&& other) noexcept;
	BLAS& operator=(BLAS&& other) noexcept;
	
	operator const VkAccelerationStructureKHR& () const { return as; }
	
	VkDeviceAddress device_address() const;

	~BLAS();

private:
	
	BLAS(Context& context, Mesh& mesh, const Buffer& vertexBuffer, const Buffer& indexBuffer);

	Buffer asBuffer;
	VkAccelerationStructureKHR as = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
};

class TLAS {
	friend class ASManager;
public:
	TLAS(){}
	TLAS(TLAS&& other) noexcept;
	TLAS& operator=(TLAS&& other) noexcept;

	operator const VkAccelerationStructureKHR& () const { return as; }

	~TLAS();

private:

	TLAS(Context& context, Scene& scene);

	Buffer asBuffer;
	pstd::vector<BLAS> blas;
	VkAccelerationStructureKHR as = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;

};



class ASManager {
	friend class BLAS;
	friend class TLAS;
public:
	ASManager(Context& context);

	~ASManager();

	void init();

	TLAS get_tlas(Scene& scene);

private:

	void load_function();

	static inline PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;

	static inline PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;

	static inline PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;

	static inline PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;

	static inline PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;



	Context& _context;
};

#endif // !VULKAN_ACCELERATION_STRUCTURE_MANAGER

