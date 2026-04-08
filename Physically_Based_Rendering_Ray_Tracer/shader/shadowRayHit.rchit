#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : require

#include "./generated/RAY_TRACING_header.glslh"

layout(location = 1) rayPayloadInEXT ShadowRayInfo srd;

void main(){
	float hitT = gl_HitTEXT;

	if(abs(hitT - srd.targetT) < 1e-4)
		srd.shadowBlocked = false;
}