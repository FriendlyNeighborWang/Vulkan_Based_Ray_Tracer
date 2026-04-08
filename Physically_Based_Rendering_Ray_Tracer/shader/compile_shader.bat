echo ===================== RAY_TRACING Pipeline =====================
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\rayGen.rgen.spv rayGen.rgen
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\rayMiss.rmiss.spv rayMiss.rmiss
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\closestHit.rchit.spv closestHit.rchit
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\alphaTest.rahit.spv alphaTest.rahit
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\shadowRayMiss.rmiss.spv shadowRayMiss.rmiss
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\shadowRayHit.rchit.spv shadowRayHit.rchit
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\shadowRayAnyHit.rahit.spv shadowRayAnyHit.rahit
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\ReSTIR.rgen.spv ReSTIR.rgen

echo ===================== G_BUFFER Pipeline =====================
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\gBuffer.rahit.spv .\gBuffer\gBuffer.rahit
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\gBuffer.rchit.spv .\gBuffer\gBuffer.rchit
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\gBuffer.rgen.spv .\gBuffer\gBuffer.rgen
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\gBuffer.rmiss.spv .\gBuffer\gBuffer.rmiss

echo ===================== Reservoir Pipelines (Compute) =====================
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\init_reservoir.comp.spv .\reservoir\init_reservoir.comp
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\temporal_reuse.comp.spv .\reservoir\temporal_reuse.comp
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\spatial_reuse.comp.spv .\reservoir\spatial_reuse.comp

echo ===================== TONE_MAPPING Pipeline (Compute) =====================
E:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe -g -Od --target-env vulkan1.3 -o spv\tone_mapping.comp.spv tone_mapping.comp

pause