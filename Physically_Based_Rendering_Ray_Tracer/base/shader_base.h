#ifndef SHADER_BASE_COMMON_H
#define SHADER_BASE_COMMON_H

#ifdef __cplusplus
#include <cstdint>
using uint = uint32_t;
#endif

struct PushConstants {
	uint sample_batch;
};

#define BINDING_RENDERING_TARGET_IMAGE 0
#define BINDING_TLAS 1
#define BINDING_VERTICES 2
#define BINDING_INDICES 3

struct FormatTransPushConstants {
	float exposure;
};

#define BINDING_COMPUTE_FORMAT_TRANSFER_HDR_IMAGE 0
#define BINDING_COMPUTE_FORMAT_TRANSFER_LDR_IMAGE 1

#endif // !SHADER_BASE_COMMON_H
