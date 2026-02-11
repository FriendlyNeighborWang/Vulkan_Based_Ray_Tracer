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

#define RAY_TRACING_DYNAMIC_SET 0
#define RAY_TRACING_IMAGE_SET 1
#define RAY_TRACING_UNIFORM_SET 2

#define COMPUTE_TONE_MAPPING_SET 0



#define BINDING_RAY_TRACING_SCENE_DYNAMIC_INFO 0

#define BINDING_RAY_TRACING_RENDERING_TARGET_IMAGE 0

#define BINDING_RAY_TRACING_SCENE_STATIC_INFO 0
#define BINDING_RAY_TRACING_TLAS 1
#define BINDING_RAY_TRACING_VERTICES 2
#define BINDING_RAY_TRACING_INDICES 3
#define BINDING_RAY_TRACING_MATERIAL 4
#define BINDING_RAY_TRACING_GEOMETRY 5
#define BINDING_RAY_TRACING_LIGHT 6


#define BINDING_COMPUTE_TONE_MAPPING_HDR_IMAGE 0
#define BINDING_COMPUTE_TONE_MAPPING_LDR_IMAGE 1

// Material Type
#define MATERIAL_TYPE_COOK_TORRANCE 0

// Light Type
#define LIGHT_TYPE_MESH 0 

#endif // !SHADER_BASE_COMMON_H
