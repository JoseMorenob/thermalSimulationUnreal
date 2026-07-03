# IRSimClean

Small Unreal Engine 5 prototype for generating a simplified IR view from a standard 3D scene.

This project is part of a master's thesis focused on infrared sensor simulation. Right now the goal is not to be a full sensor model, but to keep a clean and understandable pipeline that goes from scene temperature data to LWIR radiance and capture.

## What it does

- Assigns simple thermal parameters to scene actors
- Computes band radiance in LWIR (8-12 um)
- Adds a basic reflected/background term
- Applies simple atmospheric attenuation
- Captures the result through a `SceneCapture2D`

## Project structure

- `Source/IRSimClean/` main Unreal module
- `Plugins/IRSimPlugin/` small plugin layer
- `Content/` materials and scene assets used in the prototype
- `Config/` Unreal project configuration

## Notes

This is a research prototype. The current version is intentionally simple and focused on the working pipeline rather than on a full production-ready IR camera simulation.

## Requirements

- Unreal Engine 5.6

## Open

Open `IRSimClean.uproject` with Unreal Engine and build from the editor if needed.

