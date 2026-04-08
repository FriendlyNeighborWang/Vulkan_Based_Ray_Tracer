#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../generated/G_BUFFER_header.glslh"

#include "../skybox/skybox.glslh"

layout(location = 0) rayPayloadInEXT uint payloadPixelIndex;

void main(){
	gBuffer_current[payloadPixelIndex].worldPosition = vec3(0.0);
	gBuffer_current[payloadPixelIndex].compactShadingNormal = vec2(0.0);
	gBuffer_current[payloadPixelIndex].compactGeoNormal = vec2(0.0);

	MaterialSample mat;
	mat.materialType = 0;
	mat.albedo = vec3(0.0);
	mat.metallic = 0.0;
	mat.roughness = 0.0;
	mat.transmissionFactor = 0.0;
	mat.ior = 1.0;
	mat.specularFactor = 0.0;
	mat.specular = vec3(0.0);

	if(staticInfo.ifHasSkyBox == 1){
		vec3 dir = normalize(gl_WorldRayDirectionEXT);
		mat.emission = evaluateSkyBox(dir);
	}else{
		mat.emission = vec3(-1.0);
	}

	gBuffer_current[payloadPixelIndex].material = mat;
}