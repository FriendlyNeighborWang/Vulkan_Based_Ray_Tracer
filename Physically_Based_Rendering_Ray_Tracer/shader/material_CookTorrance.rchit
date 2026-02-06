#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require
#include "closestHitCommon.glslh"
#include "material_CookTorrance.glslh"



void main(){	
	HitInfo hitInfo = getObjectHitInfo();

	// Sampling Ray direciton & weight
	vec3 N = hitInfo.worldNormal;
	vec3 V = -gl_WorldRayDirectionEXT;
	Material mat = hitInfo.material;

	vec3 weight;
	vec3 dir = sampleBRDF(
		N, 
		V, 
		mat.albedo.rgb,
		mat.metallic,
		mat.roughness,
		pld.rngState,
		weight
	);

	pld.radiance += pld.throughput * hitInfo.material.emission;
	pld.throughput *= weight;
	pld.rayOrigin = offsetPositionAlongNormal(hitInfo.worldPosition, hitInfo.worldNormal);
	pld.rayDirection = dir;
	pld.rayHitSky = false;
	
}