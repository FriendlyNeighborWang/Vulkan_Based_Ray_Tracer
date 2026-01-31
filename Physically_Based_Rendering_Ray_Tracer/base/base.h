#ifndef PBRT_BASE
#define PBRT_BASE
// This head file is for class clarification and macro

#include <exception>
#include <vulkan/vulkan.h>

#include "shader_base.h"
#include "util/log.h"

// --------------------Macro--------------------


#ifdef USE_DOUBLE_AS_FLOAT
using Float = double;
#else
using Float = float;
#endif // USE_DOUBLE_AS_FLOAT

// Debug && Log

inline void InitLogging() {
	REGISTER_LOG("scene_loader", true);
}

const uint32_t WIDTH = 1200;
const uint32_t HEIGHT = 900;
const uint32_t MAX_FRAMES_IN_FLIGHT = 2;


// --------------------Class--------------------
namespace pstd {
	// Memory
	class LinearAllocator;
	class HeapAllocator;
}

// Vec
struct Point2f;
struct Point2i;
struct Point3f;
struct Vector2f;
struct Vector3f;
struct Normal;

// Geometry
struct Mesh;
struct MeshInstance;

// Util
class TimerManager;


// Vulkan Layer
class Context;
class SwapChain;
class Image;
class Buffer;
class CommandBuffer;
class CommandPool;
class VkMemoryAllocator;
class ShaderManager;
class DescriptorManager;
class BLAS;
class TLAS;
class ASManager;
class RTPipeline;
class ComputePipeline;
class Fence;
class Semaphore;




#endif // !PBRT_BASE

