# IRSimClean — Simulación de sensor infrarrojo en Unreal Engine 5

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5-0E1128?logo=unrealengine) ![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus) ![HLSL](https://img.shields.io/badge/HLSL-shaders-5C2D91)

Prototipo en tiempo real para simular la formación de imagen de una cámara
infrarroja en **Unreal Engine 5.6**. El proyecto forma parte del TFM de José
Moreno Barbero (Máster CGRVS, U-tad) y separa deliberadamente la física, la
integración de Unreal y la visualización del detector.

## Estructura

- `Plugins/IRSimPlugin`: modelo de radiancia LWIR, materiales térmicos, buffers
  auxiliares y captura `SceneCapture2D`.
- `Source/IRPipelineCore`: biblioteca estática que representa la respuesta del
  detector (calibración, ruido, AGC y paleta).
- `Source/IRSimClean`: capa de demostración que entrega la radiancia de Unreal
  a `IRPipelineCore` y muestra el resultado.
- `Content/Maps/IRBufferValidation`: escena mínima exclusiva de las pruebas
  automáticas; no es una escena de demostración.

## Ejecutar

1. Abre `IRSimClean.uproject` con Unreal Engine 5.6 y recompila los módulos C++.
2. Carga una escena que contenga `ARadianceCaptureActor`,
   `AIRSceneEnvironmentActor` y superficies con `UIRThermalSurfaceComponent`.
3. Inicia Play. El panel de demostración aparece automáticamente; con `I` se
   muestra u oculta. Permite cambiar temperatura, atmósfera, banda espectral y
   buffers de depuración sin modificar la física del núcleo.

## Pruebas

En el editor, abre **Tools > Test Automation** y ejecuta el filtro
`IRSimClean.Radiance`. Las pruebas comprueban radiancia CPU/GPU, Beer–Lambert,
buffers auxiliares y Fresnel sobre el mapa `IRBufferValidation`.

## Alcance

La simulación visual usa una aproximación de banda LWIR. No sustituye una
calibración metrológica de un sensor real: las pruebas verifican la coherencia
entre la implementación CPU y la ruta de renderizado del proyecto.
