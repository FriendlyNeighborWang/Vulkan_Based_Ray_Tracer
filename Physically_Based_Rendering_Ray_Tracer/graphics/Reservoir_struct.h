#ifndef GRAPHICS_RESERVOIR_H
#define GRAPHICS_RESERVOIR_H

#include "base/base.h"
#include "util/Vec.h"

struct LightSample {
	Vector3f scalar;
	int lightIdx;
	Vector3f vector;
	float sourcePdf;
};

struct Reservoir {
	LightSample sample;
	float wSum = 0.0f;
	float targetPdf = 0.0f;
	float W = 0.0f;
	uint32_t M = 0;
};

struct MaterialSample {
	uint32_t materialType;
	Vector3f emission;
	Vector3f albedo;
	float metallic;
	float roughness;
	float transmissionFactor;
	float ior;
	float specularFactor;
	Vector3f specular;
	uint32_t padding;
};

struct GBufferElement {
	// Position
	Vector3f worldPosition;
	uint32_t _padding;
	Vector2f compactShadingNormal;
	Vector2f compactGeoNormal;
	MaterialSample sample;
};


#endif // !GRAPHICS_RESERVOIR_H
