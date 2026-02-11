#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#include"./common/shaderCommon.glslh"

layout(location = 1) rayPayloadInEXT ShadowRayInfo srd;

void main(){
	float hitT = gl_HitTEXT;
	float epsilon = 0.01;

	if(abs(hitT - srd.targetT) < epsilon)
		srd.shadowBlocked = false;
}