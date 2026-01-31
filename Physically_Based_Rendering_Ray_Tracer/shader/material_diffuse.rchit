#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require
#include "closestHitCommon.glslh"

vec3 diffuseReflection(vec3 normal, inout uint rngState){
	const float theta = 2.0 * k_pi * stepAndOutputRNGFloat(rngState);
	const float u = 2.0 * stepAndOutputRNGFloat(rngState) - 1.0;
	const float r = sqrt(1.0 - u * u);
	const vec3 direction = normal + vec3(r * cos(theta), r * sin(theta), u);

	return normalize(direction);
}

void main(){
	
	HitInfo hitInfo = getObjectHitInfo();

	pld.color = vec3(0.7);
	pld.rayOrigin = offsetPositionAlongNormal(hitInfo.worldPosition, hitInfo.worldNormal);
	pld.rayDirection = diffuseReflection(hitInfo.worldNormal, pld.rngState);
	pld.rayHitSky = false;
}