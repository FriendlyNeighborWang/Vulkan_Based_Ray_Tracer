#ifndef PBRT_BASE
#define PBRT_BASE
// This head file is for class clarification and macro

#include <exception>
#include <vulkan/vulkan.h>



#include "shader_base.h"
#include "util/log.h"

// Debug && Log

inline void InitLogging() {
	REGISTER_LOG("scene_loader", true);
}

// --------------------Macro--------------------


#ifdef USE_DOUBLE_AS_FLOAT
using Float = double;
#else
using Float = float;
#endif // USE_DOUBLE_AS_FLOAT


const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 1200;
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

// Scene
struct SamplerData;
struct TextureData;
struct Material;
struct Light;
struct Geometry;
struct Mesh;
struct MeshInstance;
struct Scene;
class SkyBox;

// Graphics
class Renderer;
class RenderPass;
class FrameContext;
class ResourceManager;

// Util
class TimerManager;


// Vulkan Layer
class Context;
class SwapChain;
class Image;
class Buffer;
class Sampler;
class Texture;
class CommandBuffer;
class CommandPool;
class VkMemoryAllocator;
class DescriptorSetLayout;
class DescriptorSet;
class DescriptorManager;
class BLAS;
class TLAS;
class ASManager;
class ShaderModule;
class Pipeline;
class RTPipeline;
class ComputePipeline;
class PipelineManager;
class Fence;
class Semaphore;

// --------------------Resource Flag--------------------
using ResourceFlag = uint64_t;

// Update Frequency [bit 0:15]
constexpr ResourceFlag RF_STATIC = 0x0000'0000'0000'0001ULL;
constexpr ResourceFlag RF_PER_FRAME = 0x0000'0000'0000'0002ULL;


// Other Flag [bit 16: 31]
constexpr ResourceFlag RF_WINDOW_SIZE_RELATED = 0x0000'0000'0001'0000ULL;

// Mask
constexpr ResourceFlag RF_FREQUENCY_MASK = 0x0000'0000'0000'000FULL;


#endif // !PBRT_BASE

