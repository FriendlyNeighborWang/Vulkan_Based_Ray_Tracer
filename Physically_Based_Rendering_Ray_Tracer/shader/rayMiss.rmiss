#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require

#include "./generated/RAY_TRACING_header.glslh"

layout(location = 0) rayPayloadInEXT PassableInfo pld;

#include "./sampling/MIS.glslh"
#include "./skybox/skybox.glslh"


void main(){
	vec3 skyColor = vec3(0.0);

	if(staticInfo.ifHasSkyBox == 1){

		vec3 dir = normalize(gl_WorldRayDirectionEXT);
		skyColor = evaluateSkyBox(dir);

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