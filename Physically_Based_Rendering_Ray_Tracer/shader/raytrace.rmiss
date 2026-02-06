#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require
#include "shaderCommon.glslh"

layout(location = 0) rayPayloadInEXT PassableInfo pld;

void main(){
	const float rayDirY = gl_WorldRayDirectionEXT.y;

	vec3 skyColor = vec3(0.0);
	/*if(rayDirY >0.0f){
		skyColor = mix(vec3(0.5, 0.6, 0.8), vec3(0.3, 0.5, 1.0), rayDirY) * 2.0;
		// pld.color = vec3(1.0f, 0.0f, 0.0f);
	}else{
		skyColor = vec3(0.03f);
	}*/
	

	pld.radiance = pld.throughput * skyColor;
	pld.rayHitSky = true;
}