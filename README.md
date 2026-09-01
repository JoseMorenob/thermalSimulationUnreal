# IRSimClean — Infrared Sensor Simulation in Unreal Engine 5

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5-0E1128?logo=unrealengine) ![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus) ![HLSL](https://img.shields.io/badge/HLSL-shaders-5C2D91)

A real-time prototype for simulating the image formation of an infrared camera in **Unreal Engine 5**. The project explores how material thermal properties, long-wave infrared radiance and environmental effects can be translated into an interpretable sensor image for simulation and validation workflows.

## What it demonstrates

- Thermal material parameters and scene-dependent emissive behaviour
- LWIR radiance modelling in the 8–12 μm band
- Background, reflection and basic atmospheric attenuation effects
- Unreal `SceneCapture2D` integration for an in-engine sensor pipeline
- C++ and HLSL work across engine integration and rendering logic

## Why this project

Infrared simulation is useful when real sensor data is expensive, limited or unavailable. This prototype focuses on making the key physical effects visible and controllable inside a real-time 3D environment, rather than treating thermal imagery as a simple post-processing filter.

## Technology

- Unreal Engine 5
- C++
- HLSL / material and shader logic
- Real-time rendering and sensor simulation concepts

## Status

Active MSc work in progress. The repository documents a working prototype and is being developed as part of a broader interest in simulation, autonomous systems and sensor validation.

## Contact

[José Moreno Barbero](https://github.com/JoseMorenob) · C++ Software Engineer
