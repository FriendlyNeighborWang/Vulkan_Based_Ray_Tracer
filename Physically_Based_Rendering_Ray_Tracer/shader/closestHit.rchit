#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : require

#include "./generated/RAY_TRACING_header.glslh"

// Ray Payload
layout(location = 0) rayPayloadInEXT PassableInfo pld;
layout(location = 1) rayPayloadEXT ShadowRayInfo srd;

#include "./common/hitShaderCommon.glslh"
#include "./sampling/NEE.glslh"


void main(){	
	HitInfo hitInfo = getObjectHitInfo();

	vec3 V = -gl_WorldRayDirectionEXT;
	vec3 L = vec3(0.0);
	vec3 weight = vec3(0.0);
	vec3 radiance = sampleNEE(V, hitInfo, pld.lastIIInfo, weight, L, pld.ifStop, pld.rngState);


	pld.radiance += pld.throughput * radiance;
	pld.throughput *= weight;
	pld.rayOrigin = (dot(L, hitInfo.worldGeoNormal) < 0.0) ? 
		offsetPositionAlongNormal(hitInfo.worldPosition, -hitInfo.worldGeoNormal) : 
		offsetPositionAlongNormal(hitInfo.worldPosition, hitInfo.worldGeoNormal);
	pld.rayDirection = L;
}