#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require
#include "./common/closestHitCommon.glslh"
#include "./sampling/NEE.glslh"


void main(){	
	HitInfo hitInfo = getObjectHitInfo();

	// Sampling Ray direciton & weight
	vec3 N = hitInfo.worldNormal;
	vec3 V = -gl_WorldRayDirectionEXT;
	Material mat = hitInfo.material;

	vec3 weight = vec3(1.0);
	vec3 L = vec3(0.0);
	vec3 radiance = sampleNEE(N, V, hitInfo.worldPosition, mat, weight, L, pld.if_DI, pld.rngState);


	pld.radiance += pld.throughput * radiance;
	pld.throughput *= weight;
	if(weight != vec3(1.0))
		pld.rayOrigin = offsetPositionAlongNormal(hitInfo.worldPosition, hitInfo.worldNormal);
	pld.rayDirection = L;
	pld.rayHitSky = false;
}