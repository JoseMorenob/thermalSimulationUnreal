# IRSimPlugin

Runtime plugin for the IRSimClean Unreal prototype.

## Contains

- `UIRThermalSurfaceComponent`: reusable thermal surface component for any actor with a `StaticMeshComponent`.
- The validation scene uses a regular `StaticMeshActor` with `UIRThermalSurfaceComponent`.
- `AIRSceneEnvironmentActor`: shared scene, atmosphere and spectral-band settings.
- `ARadianceCaptureActor`: radiance render target capture.
- `AThermalPipelineController`: scene helper that refreshes every `UIRThermalSurfaceComponent` in the scene.
- `AIRRadianceValidationActor`: CPU/GPU validation helper.
- `IRSimClean.Radiance.CpuGpuMinimalScene`: Unreal automation test.
- C++17 radiometry core: Planck spectral radiance, band integration, surface radiance and atmospheric attenuation.
- Plugin materials under `Content/Materials`.

## Using a regular mesh

1. Add **IR Thermal Surface** to an actor that contains a `StaticMeshComponent`.
2. Set `TemperatureK`, `Emissivity`, and `DebugMaterial` to `M_ThermalSurface_Physical` for the sensor/data path. Use `M_IR_DebugDisplay` only for the player post-process visualization.
3. Add `ThermalPipelineController`, assign its environment and capture actors, then click `Refresh Pipeline`.

La automatización valida el camino único basado en `UIRThermalSurfaceComponent` contra el render target de radiancia.

The old project-module copies were moved to `Source/IRSimClean_LegacyMovedToPlugin` and are not part of the active Unreal module.
