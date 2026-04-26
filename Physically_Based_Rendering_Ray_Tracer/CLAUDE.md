# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Windows Vulkan KHR ray tracer with ReSTIR DI, Cook-Torrance BRDF, HDR skybox/point/directional/mesh lights, and an offline + realtime path. Single-binary desktop app (`main.cpp`). No CMake — builds through the MSBuild `.vcxproj` only.

## Build, Run, Shaders

- **Build:** open `Physically_Based_Rendering_Ray_Tracer.vcxproj` in Visual Studio 2022 (PlatformToolset v143, x64). Includes/libs are hardcoded to `E:\VulkanSDK\1.4.321.1` and `D:\C++Libiary\glfw-3.4.bin.WIN64` in the vcxproj — retarget for a different host.
- **Run:** the executable must run from the project root; asset paths like `./resource/...` and `./shader/spv/...` / `./shader/generated/...` are resolved relative to cwd.
- **Compile shaders:** run `shader/compile_shader.bat` (invokes `glslangValidator --target-env vulkan1.3`, outputs to `shader/spv/`). The C++ build does not rebuild SPIR-V — rerun the batch file after editing any `.rgen/.rchit/.rahit/.rmiss/.comp/.glslh` file.
- **Generated GLSL headers:** `shader/generated/*_header.glslh` are *re-emitted at runtime* by `ResourceManager::generate_header` (see `graphics/ResourceManager.cpp:750`) based on the `register_resources` calls in `main.cpp`. After changing descriptor bindings in `main.cpp`, launch the app once to regenerate the headers, then recompile the affected shaders with `compile_shader.bat`.
- **Tests:** `test/util_*_test.cpp` exist but are not compiled by the `.vcxproj` — there is no test runner target. Treat them as standalone sources unless/until one is wired up.

## Where changes happen

Scenes, skybox, and realtime vs offline are all switched by editing `main.cpp`:
- Active scene is chosen by uncommenting one `sceneLoader.LoadScene(...)` call (lines ~113-119).
- Optional skybox via `scene.register_skybox(...)` (commented out by default).
- Final line toggles `renderer.realtime_render()` vs `renderer.offline_render("result.hdr")`.

## Architecture

Three-layer design, all owned transitively by a single `Context`.

**`vk_layer/` — Vulkan RAII + sub-managers.** `Context` (`vk_layer/Context.h`) creates the instance/device/surface and owns: `VkMemoryAllocator`, `CommandPool`, `DescriptorManager`, `ASManager` (BLAS/TLAS build), `PipelineManager`, `ResourceManager`. Other units (`Buffer`, `Image`, `Texture`, `SwapChain`, `SyncObject`, `ShaderModule`, `Pipeline`/`RTPipeline`/`ComputePipeline`/`GraphicsPipeline`) are thin wrappers. Features required at device creation are registered via `Context::register_device_feature` + a validator lambda (see `main.cpp:87-97`).

**`graphics/` — scene + render orchestration.**
- `SceneLoader` (singleton, `SceneLoader::Get()`) loads glTF via tiny_gltf/assimp into a `Scene`.
- `Scene` owns CPU-side vertex/index/material/geometry/light/normal/tangent/texcoord arrays plus `SceneStaticInfo` and `SceneDynamicInfo` (camera, `viewProj`, `prevViewProj`); `get_*_buffer(context)` uploads them and returns `Buffer&`.
- `ResourceManager` is the central registry: every GPU resource is `register_resources(...)` with a name, a `ResourceFlag` bitset, the list of `Pipeline*` that consume it, a `VkDescriptorType`, and a `GLSLLayoutInfo` (block/member/array metadata). `build()` allocates descriptor sets *and* writes the per-pipeline `shader/generated/<PIPELINE>_header.glslh`.
- `Renderer` runs the frame. Realtime pass order: `G_BUFFER` (RT) → `INITIALIZE_RESERVOIR` → `TEMPORAL_REUSE` → `SPATIAL_REUSE` → `RAY_TRACING` (`ReSTIR.rgen`) → `TONE_MAPPING`. Offline uses the same pipelines without swapchain presentation.
- `InputManager` mutates `Scene::dynamicInfo` (camera) each frame; `Window` wraps GLFW.

**`base/`, `util/`.**
- `base/base.h` — forward decls, `Float` typedef, `WIDTH/HEIGHT/MAX_FRAMES_IN_FLIGHT`, and the `ResourceFlag` bit layout (`RF_STATIC | RF_PER_FRAME | RF_TEMPORAL` update frequency × `RF_BIND_DESCRIPTOR | RF_BIND_VERTEX` bind point × `RF_WINDOW_SIZE_RELATED` property).
- `base/shader_base.h` — shared between C++ and GLSL via `#ifdef __cplusplus`; holds push-constant structs (`PushConstants`, `ReservoirPushConstants`, `ToneMappingPushConstants`) and the `MATERIAL_TYPE_*` / `LIGHT_TYPE_*` / `ALPHA_MODE_*` enums. Edit this file to keep CPU structs and shader structs in lockstep.
- `util/pstd.h` — in-house `vector`, `optional`, `span`, `tuple`, `array` built on `util/memory.h` (`LinearAllocator`, `HeapAllocator`, TLSF). Most signatures in this project use `pstd::vector` rather than `std::vector`; a stray `std::vector` in a hot path is usually a bug.
- `util/log.h` — `REGISTER_LOG` channel system (the `scene_loader` channel is registered by `InitLogging()` in `base.h`).

**Shader tree.** `shader/common/*.glslh` (`shaderCommon`, `hitShaderCommon`, `util`), `shader/material/` (Cook-Torrance + interface), `shader/light/` (Directional/Point/Mesh + interface), `shader/sampling/` (NEE, MIS), `shader/reservoir/` (ReSTIR init/temporal/spatial reuse), `shader/gBuffer/`, `shader/skybox/`. Every shader `#include`s the matching `shader/generated/<PIPELINE>_header.glslh` for its descriptor bindings.

## Conventions worth knowing

- Resources are addressed by **string name** (`"HDR_IMAGE"`, `"RESERVOIR_BUFFER"`, etc.) across `ResourceManager`, `PipelineManager`, and `DescriptorManager`. Renaming one means renaming all call sites.
- Per-frame resources registered with `RF_PER_FRAME` are keyed `name + frame_idx` (see `Renderer::updateRenderingSetting` in `graphics/Renderer.cpp:66`).
- Ray-tracing extension function pointers (`vkCmdTraceRaysKHR`, `vkCreateRayTracingPipelinesKHR`, `vkGetRayTracingShaderGroupHandlesKHR`) are loaded lazily in the constructors of `Renderer` / `RTPipeline` — include `vulkan.h` transitively via `base/base.h`, don't re-declare them.
- Third-party single-header macros (`TINYGLTF_IMPLEMENTATION`, `STB_IMAGE_IMPLEMENTATION`, `STB_IMAGE_WRITE_IMPLEMENTATION`) are defined exactly once in `main.cpp`; don't add them elsewhere.
