#ifndef VULKAN_ACCELERATION_STRUCTURE_MANAGER
#define VULKAN_ACCELERATION_STRUCTURE_MANAGER

#include "base/base.h"
#include "graphics/Mesh.h"
#include "Context.h"

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
	
	BLAS(ASManager* address, Mesh& mesh);

	Buffer asBuffer;
	VkAccelerationStructureKHR as = VK_NULL_HANDLE;

	ASManager* pasm = nullptr;
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

	TLAS(ASManager* address, pstd::vector<Mesh>& meshes, pstd::vector<MeshInstance>& instances);

	Buffer asBuffer;
	pstd::vector<BLAS> blas;
	VkAccelerationStructureKHR as = VK_NULL_HANDLE;

	ASManager* pasm = nullptr;

};



class ASManager {
	friend class BLAS;
	friend class TLAS;
public:
	ASManager(Context& context);

	~ASManager();

	void init();

	TLAS get_tlas(pstd::vector<Mesh>& meshes, pstd::vector<MeshInstance>& instances);

private:

	void load_function();

	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;

	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;

	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;

	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;

	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;	


	Buffer asBuffer;
	Context& _context;
};

#endif // !VULKAN_ACCELERATION_STRUCTURE_MANAGER

