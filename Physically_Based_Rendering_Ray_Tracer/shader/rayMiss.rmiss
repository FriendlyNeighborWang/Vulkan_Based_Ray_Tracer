#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require
#include "../base/shader_base.h"
#include "./common/shaderCommon.glslh"
#include "./sampling/MIS.glslh"

layout(binding = BINDING_PIPELINE_RAY_TRACING_DESCRIPTOR_SET_STATIC_SCENE_STATIC_INFO, set = PIPELINE_RAY_TRACING_DESCRIPTOR_SET_STATIC_SET) uniform SceneStaticInfo{
	uint lightCount;
	float totalLightPower;
	uint bufferFlags;
	uint ifHasSkyBox;
} staticInfo;

layout(binding = BINDING_PIPELINE_RAY_TRACING_DESCRIPTOR_SET_STATIC_SKYBOX_TEXTURE_ARRAY, set = PIPELINE_RAY_TRACING_DESCRIPTOR_SET_STATIC_SET) uniform sampler2D skyboxTextures[];

layout(location = 0) rayPayloadInEXT PassableInfo pld;



float evaluateSkyBoxPDF(vec3 direction){
	ivec2 envSize = textureSize(skyboxTextures[0], 0);
	uint width = envSize.x;
	uint height = envSize.y;

	float theta = acos(clamp(direction.y, -1.0, 1.0));
	float phi = atan(direction.z, direction.x);
	if(phi < 0.0) phi += 2.0 * PI;

	float u = phi / (2.0 * PI);
	float v = theta / PI;

	uint x = clamp(uint(u * float(width)), 0, width - 1);
	uint y = clamp(uint(v * float(height)), 0, height - 1);


	float cdfCurr = texelFetch(skyboxTextures[nonuniformEXT(1)], ivec2(x,y), 0).r;
	float cdfPrev = (x > 0) ? texelFetch(skyboxTextures[nonuniformEXT(1)], ivec2(x - 1,y), 0).r : 0.0;
	float pConditional = cdfCurr - cdfPrev;

	float mCdfCurr = texelFetch(skyboxTextures[nonuniformEXT(2)], ivec2(y, 0), 0).r;
	float mCdfPrev = (y > 0) ? texelFetch(skyboxTextures[nonuniformEXT(2)], ivec2(y - 1, 0), 0).r : 0.0;
	float pMarginal = mCdfCurr - mCdfPrev;


	float sinTheta = sin(theta);
	if(sinTheta < 1e-5) return 0.0;

	return (pConditional * pMarginal * float(width) * float(height)) / (2.0 * PI * PI * sinTheta);
}


void main(){
	vec3 skyColor = vec3(0.0);

	if(staticInfo.ifHasSkyBox == 1){

		vec3 dir = normalize(gl_WorldRayDirectionEXT);

		float theta = acos(clamp(dir.y, -1.0, 1.0));
		float phi = atan(dir.z, dir.x);
		if(phi < 0.0) phi += 2.0 * PI;

		vec2 uv = vec2(phi / (2.0 * PI), theta / PI);
		skyColor = texture(skyboxTextures[0], uv).rgb;

		if(pld.lastIIInfo.available){
			float pdf_light = evaluateSkyBoxPDF(dir);
			float pdf_bsdf = pld.lastIIInfo.bsdfPdf;
			float MIS_weight = powerHeuristicWeight(pdf_bsdf, pdf_light);

			skyColor = MIS_weight * skyColor;
		}
	}
	

	pld.radiance += pld.throughput * skyColor;
	pld.ifStop = true;
}