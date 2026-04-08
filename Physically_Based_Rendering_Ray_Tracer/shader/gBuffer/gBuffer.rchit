#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : require

#include "../generated/G_BUFFER_header.glslh" 

layout(location = 0) rayPayloadInEXT uint payloadPixelIndex;

#include "../common/hitShaderCommon.glslh"
#include "../material/material_sample.glslh"

void main(){
	HitInfo hitInfo = getObjectHitInfo();
	GeometryStruct geo = geometries[hitInfo.globalGeometryIdx];

	gBuffer_current[payloadPixelIndex].worldPosition = hitInfo.worldPosition;
	gBuffer_current[payloadPixelIndex].compactGeoNormal = encodeOct(hitInfo.worldGeoNormal);
	gBuffer_current[payloadPixelIndex].compactShadingNormal = encodeOct(hitInfo.worldShadingNormal);
	gBuffer_current[payloadPixelIndex].material = getMaterialSample(geo.materialIdx, hitInfo.uv);
}