#ifndef SHADER_BASE_COMMON_H
#define SHADER_BASE_COMMON_H

#ifdef __cplusplus
#include <cstdint>
using uint = uint32_t;
#endif


// Push Constant

struct PushConstants {
	uint sample_batch;
};

struct ToneMappingPushConstants {
	float exposure;
};

// Descriptor set & Binding

#define RAY_TRACING_IMAGE_SET 0 
#define RAY_TRACING_UNIFORM_SET 1
#define COMPUTE_TONE_MAPPING_SET 0



#define BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE 0


#define BINDING_RAY_TRACING_TLAS 0
#define BINDING_RAY_TRACING_VERTICES 1
#define BINDING_RAY_TRACING_INDICES 2
#define BINDING_RAY_TRACING_MATERIAL 3
#define BINDING_RAY_TRACING_GEOMETRY 4


#define BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE 0
#define BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE 1

#endif // !SHADER_BASE_COMMON_H
