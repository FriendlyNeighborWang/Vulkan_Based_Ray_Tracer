#ifndef SHADER_BASE_COMMON_H
#define SHADER_BASE_COMMON_H

#ifdef __cplusplus
#include <cstdint>
using uint = uint32_t;
#endif


// Push Constant

struct PushConstants {
	uint current_frame;
	uint sample_batch;
};

struct ToneMappingPushConstants {
	float exposure;
};

// Descriptor set & Binding

#include "generated_descriptor_bindings.h"


// Buffer Flag
#define BUFFER_FLAG_HAS_NORMALS 0x01
#define BUFFER_FLAG_HAS_TANGENTS 0x02
#define BUFFER_FLAG_HAS_TEXCOORDS 0x04

// Material Type
#define MATERIAL_TYPE_COOK_TORRANCE 0

// Light Type
#define LIGHT_TYPE_MESH 0 
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_SPOT 3
#define LIGHT_TYPE_SKYBOX 4


// Alpha Mode
#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_BLEND 2

#endif // !SHADER_BASE_COMMON_H
