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


const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;
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
class Window;
class Renderer;
class InputManager;
class ResourceManager;

struct Reservoir;
struct LightSample;
struct MaterialSample;
struct GBufferElement;
struct GLSLLayoutInfo;

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
class GraphicsPipeline;
class PipelineManager;
class Fence;
class Semaphore;

// --------------------Resource Flag--------------------
using ResourceFlag = uint64_t;

// [bit 0:3] Update Frequency (hex digit 0)
constexpr ResourceFlag RF_STATIC = 0x1ULL;
constexpr ResourceFlag RF_PER_FRAME = 0x2ULL;
constexpr ResourceFlag RF_TEMPORAL = 0x4ULL;

// [bit 4:7] Bind Point (hex digit 1)
constexpr ResourceFlag RF_BIND_DESCRIPTOR = 0x10ULL;
constexpr ResourceFlag RF_BIND_VERTEX = 0x20ULL;

// [bit 8:11] Property (hex digit 2)
constexpr ResourceFlag RF_WINDOW_SIZE_RELATED = 0x100ULL;

// Mask
constexpr ResourceFlag RF_FREQUENCY_MASK = 0x000FULL;
constexpr ResourceFlag RF_BINDPOINT_MASK = 0x00F0ULL;
constexpr ResourceFlag RF_PROPERTY_MASK = 0x0F00ULL;

#endif // !PBRT_BASE

