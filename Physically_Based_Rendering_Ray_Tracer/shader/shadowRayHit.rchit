#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#include"./common/shaderCommon.glslh"

layout(location = 1) rayPayloadInEXT ShadowRayInfo srd;

void main(){
	float hitT = gl_HitTEXT;

	if(abs(hitT - srd.targetT) < 1e-4)
		srd.shadowBlocked = false;
}