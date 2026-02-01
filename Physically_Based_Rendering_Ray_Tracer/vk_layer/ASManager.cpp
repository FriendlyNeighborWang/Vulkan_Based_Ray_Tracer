#include "ASManager.h"

BLAS::BLAS(BLAS&& other) noexcept:pasm(other.pasm),asBuffer(std::move(other.asBuffer)), as(other.as){
	other.as = VK_NULL_HANDLE;
	other.pasm = nullptr;
}
BLAS& BLAS::operator=(BLAS&& other) noexcept {
if (this != &other) {
	if (as != VK_NULL_HANDLE)pasm->vkDestroyAccelerationStructureKHR(pasm->_context, as, nullptr);
}

	as = other.as;
	asBuffer = std::move(other.asBuffer);
	pasm = other.pasm;
	
	other.as = VK_NULL_HANDLE;
	other.pasm = nullptr;

	return *this;
}

BLAS::~BLAS() {
	if (as != VK_NULL_HANDLE)pasm->vkDestroyAccelerationStructureKHR(pasm->_context, as, nullptr);
}

BLAS::BLAS(ASManager* address, Mesh& mesh, const Buffer& vertexBuffer, const Buffer& indexBuffer):pasm(address) {
	VkDeviceAddress vertexBufferAddress = vertexBuffer.device_address();
	VkDeviceAddress indexBufferAddress = indexBuffer.device_address();

	// Geometry Setting
	pstd::vector<VkAccelerationStructureGeometryKHR> geometries;
	pstd::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
	pstd::vector<uint32_t> primitiveCounts;

	geometries.reserve(mesh.geometries.size());
	rangeInfos.reserve(mesh.geometries.size());
	primitiveCounts.reserve(mesh.geometries.size());

	for (const auto& geo : mesh.geometries) {
		VkAccelerationStructureGeometryTrianglesDataKHR trianglesData{};
		trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		trianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		trianglesData.vertexStride = sizeof(Vector3f);
		trianglesData.vertexData.deviceAddress = vertexBufferAddress;
		trianglesData.maxVertex = mesh.firstVertex + geo.firstVertex + geo.vertexCount - 1;
		trianglesData.indexType = VK_INDEX_TYPE_UINT32;
		trianglesData.indexData.deviceAddress = indexBufferAddress;
		trianglesData.pNext = nullptr;

		VkAccelerationStructureGeometryKHR geometry{};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geometry.geometry.triangles = trianglesData;

		geometries.push_back(geometry);

		VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
		rangeInfo.firstVertex = mesh.firstVertex + geo.firstVertex;
		rangeInfo.primitiveCount = geo.primitiveCount;
		rangeInfo.primitiveOffset = (mesh.firstIndex + geo.firstIndex) * sizeof(uint32_t);
		rangeInfo.transformOffset = 0;

		rangeInfos.push_back(rangeInfo);
		primitiveCounts.push_back(geo.primitiveCount);
	}

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.geometryCount = static_cast<uint32_t>(geometries.size());
	buildInfo.pGeometries = geometries.data();

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pasm->vkGetAccelerationStructureBuildSizesKHR(
		pasm->_context,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildInfo,
		primitiveCounts.data(),
		&sizeInfo
	);
	
	// Acceleration Buffer
	asBuffer = pasm->_context.memAllocator().create_buffer(
		sizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// Build AS
	VkAccelerationStructureCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.buffer = asBuffer;
	createInfo.size = sizeInfo.accelerationStructureSize;
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	if (pasm->vkCreateAccelerationStructureKHR(pasm->_context, &createInfo, nullptr, &as) != VK_SUCCESS)
		throw std::runtime_error("ASManager::Failed to create BLAS");

	// Create Scratch Buffer
	Buffer scratchBuffer = pasm->_context.memAllocator().create_buffer(
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	VkDeviceAddress scratchAddress = scratchBuffer.device_address();

	buildInfo.dstAccelerationStructure = as;
	buildInfo.scratchData.deviceAddress = scratchAddress;

	// Command
	const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = rangeInfos.data();
	
	CommandBuffer cmdBuffer = pasm->_context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);

	pasm->vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

	cmdBuffer.end_and_submit(pasm->_context.gc_queue(), true);
}

VkDeviceAddress BLAS::device_address() const {
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addressInfo.accelerationStructure = as;
	return pasm->vkGetAccelerationStructureDeviceAddressKHR(pasm->_context, &addressInfo);
}

TLAS::TLAS(TLAS&& other) noexcept : pasm(other.pasm), asBuffer(std::move(other.asBuffer)), blas(std::move(other.blas)), as(other.as) {
	other.as = VK_NULL_HANDLE;
	other.pasm = nullptr;
}

TLAS& TLAS::operator=(TLAS&& other) noexcept {
if (this != &other) {
if (as != VK_NULL_HANDLE)pasm->vkDestroyAccelerationStructureKHR(pasm->_context, as, nullptr);
}

	as = other.as;
	blas = std::move(other.blas);
	asBuffer = std::move(other.asBuffer);
	pasm = other.pasm;

	other.as = VK_NULL_HANDLE;
	other.blas.clear();
	other.pasm = nullptr;

	return *this;
}

TLAS::~TLAS() {
	if (as != VK_NULL_HANDLE)pasm->vkDestroyAccelerationStructureKHR(pasm->_context, as, nullptr);
}

TLAS::TLAS(ASManager* address, Scene& scene):pasm(address) {

	pstd::vector<VkAccelerationStructureInstanceKHR> vkInstances;

	

	// Build BLAS
	blas.resize(scene.meshes.size());
	for (uint32_t i = 0; i < scene.meshes.size(); ++i) {
		blas[i] = BLAS(
			address,
			scene.meshes[i],
			scene.get_vertex_buffer(pasm->_context.memAllocator()),
			scene.get_index_buffer(pasm->_context.memAllocator())
		);
	}

	// Instance Buffer
	for (const auto& instance : scene.instances) {
		VkAccelerationStructureInstanceKHR vkInst{};

		const glm::mat4& m = instance.transform;
		vkInst.transform = {
			m[0][0], m[1][0], m[2][0], m[3][0],
			m[0][1], m[1][1], m[2][1], m[3][1],
			m[0][2], m[1][2], m[2][2], m[3][2]
		};

		vkInst.instanceCustomIndex = instance.instanceCustomIndex;
		vkInst.mask = 0xFF;
		vkInst.instanceShaderBindingTableRecordOffset = instance.instanceShaderBindingTableRecordOffset;
		vkInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		vkInst.accelerationStructureReference = blas[instance.meshIndex].device_address();

		vkInstances.push_back(vkInst);
	}

	VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * vkInstances.size();

	Buffer instanceBuffer = pasm->_context.memAllocator().create_buffer(
		instanceBufferSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	pasm->_context.memAllocator().copy_to_buffer_directly(vkInstances.data(), instanceBuffer);

	// Geometry Setting
	VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
	instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instancesData.data.deviceAddress = instanceBuffer.device_address();

	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances = instancesData;

	// Build Info
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry;


	uint32_t instanceCount = static_cast<uint32_t>(vkInstances.size());
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pasm->vkGetAccelerationStructureBuildSizesKHR(
		pasm->_context,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildInfo,
		&instanceCount,
		&sizeInfo
	);

	asBuffer = pasm->_context.memAllocator().create_buffer(
		sizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// Create AS
	VkAccelerationStructureCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.buffer = asBuffer;
	createInfo.size = sizeInfo.accelerationStructureSize;
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;


	if (pasm->vkCreateAccelerationStructureKHR(pasm->_context, &createInfo, nullptr, &as) != VK_SUCCESS)
		throw std::runtime_error("ASManager::Failed to create TLAS");

	// Create Scratch Buffer
	Buffer scratchBuffer = pasm->_context.memAllocator().create_buffer(
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	VkDeviceAddress scratchAddress = scratchBuffer.device_address();

	buildInfo.dstAccelerationStructure = as;
	buildInfo.scratchData.deviceAddress = scratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
	rangeInfo.primitiveCount = instanceCount;
	rangeInfo.primitiveOffset = 0;
	rangeInfo.firstVertex = 0;
	rangeInfo.transformOffset = 0;

	const auto* pRangeInfo = &rangeInfo;

	CommandBuffer cmdBuffer = pasm->_context.cmdPool().get_command_buffer();
	cmdBuffer.begin(true);

	pasm->vkCmdBuildAccelerationStructuresKHR(
		cmdBuffer,
		1, &buildInfo,
		&pRangeInfo
	);

	cmdBuffer.end_and_submit(pasm->_context.gc_queue(), true);
}


ASManager::ASManager(Context& context) :_context(context){}

ASManager::~ASManager() {
	LOG_STREAM("ASManager") << "ASManager has been deconstructed" << std::endl;
}

void ASManager::init() {
	load_function();
}

void ASManager::load_function() {
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
		vkGetDeviceProcAddr(_context, "vkGetAccelerationStructureBuildSizesKHR"));

	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
		vkGetDeviceProcAddr(_context, "vkCreateAccelerationStructureKHR"));

	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
		vkGetDeviceProcAddr(_context, "vkDestroyAccelerationStructureKHR"));

	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
		vkGetDeviceProcAddr(_context, "vkCmdBuildAccelerationStructuresKHR"));

	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
		vkGetDeviceProcAddr(_context, "vkGetAccelerationStructureDeviceAddressKHR"));
}

TLAS ASManager::get_tlas(Scene& scene) {
	return TLAS(this, scene);
}