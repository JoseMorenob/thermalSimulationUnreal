# Integración del pipeline de radiancia

Esta guía describe la estructura del repositorio público `thermalSimulationUnreal`, donde `IRSimClean.uproject` se encuentra en la raíz. El objetivo es conectar una escena con la radiancia física y permitir que otra etapa la consuma.

## Compatibilidad y dependencias

- Proyecto de demostración: Unreal Engine 5.6, Windows x64.
- Plugin: `Plugins/IRSimPlugin`, con materiales y biblioteca `ir_core.lib` para Win64.
- Núcleo independiente: `lib/`, C++17 y CMake 3.20 o posterior.
- Procesamiento de detector: adaptador en `Source/IRSimClean/` y dependencia separada en `Source/IRPipelineCore/`. Se incluyen su cabecera y biblioteca estática, sin el código de implementación de esta última.

## Primera ejecución

Abre `IRSimClean.uproject` y recompila los módulos si Unreal lo solicita. Carga `/Game/Maps/IRRadianceDemo`, inicia Play y pulsa `I` para desplegar el panel. La escena `/Game/Maps/IRBufferValidation` está destinada a comprobaciones; `IRCarScene`, `UAV` y `PIPES` contienen demostraciones adicionales.

## Preparar una escena propia

1. Copia la carpeta completa `Plugins/IRSimPlugin` a `<TuProyecto>/Plugins/IRSimPlugin` y habilita el plugin.
2. Activa **Show Plugin Content** en el navegador de contenido de Unreal.
3. Añade un `IRSceneEnvironmentActor` para la banda espectral, aire, cielo y extinción atmosférica.
4. Añade un `RadianceCaptureActor` y configura posición, resolución y opciones de captura. Si debe seguir una cámara, configura las propiedades correspondientes de seguimiento.
5. Añade un `ThermalPipelineController`. Asigna explícitamente su `SceneEnvironmentActor`, `RadianceCaptureActor` y `DefaultThermalMaterial`.
6. Usa `/IRSimPlugin/Materials/M_ThermalSurface` como material térmico del controlador.
7. En cada actor con una malla estática que deba participar, añade `IRThermalSurfaceComponent`. Asigna `TargetMesh` si el actor contiene varias mallas; configura temperatura, identificador e índice de refracción complejo.
8. Ejecuta `RefreshPipeline` después de configurar las referencias. Inicia Play y comprueba la captura y los buffers.

El controlador distribuye el entorno a las superficies, asigna el material configurado y refresca la captura. La versión actual aplica el material térmico a las mallas participantes: conservar una ruta RGB independiente y sincronizada es trabajo del siguiente hito.

## Contrato del buffer físico

| Propiedad | Contrato |
|---|---|
| Acceso | `GetRadianceRenderTarget()` o evento `OnRadianceFrameCaptured` |
| Tipo | `UTextureRenderTarget2D` |
| Formato | `RGBA16F`, lineal, `sRGB=false`, gamma 1 |
| Magnitud | Radiancia integrada en banda en el canal R |
| Unidad | W·m⁻²·sr⁻¹ |
| Banda por defecto | LWIR, 8–14 µm |
| Escritura desde consumidores | El consumidor conserva su salida en otro target |

No uses el color de la vista de depuración como entrada física del detector. La ventana de visualización y los mapas de color son una representación de la radiancia.

## Conectar otro módulo

Añade `IRSimPlugin` a las dependencias de tu módulo en su archivo `.Build.cs`. Usa una dependencia pública si expones tipos del plugin en cabeceras públicas; en caso contrario, usa una privada.

```csharp
PrivateDependencyModuleNames.AddRange(new[]
{
    "Core", "CoreUObject", "Engine", "IRSimPlugin"
});
```

En Blueprint, enlaza un evento a `OnRadianceFrameCaptured` del actor de captura y consume el `PhysicalRadianceTarget` recibido. En C++, la señal es un delegado dinámico multicast; el receptor debe ser una función `UFUNCTION` compatible que reciba `UTextureRenderTarget2D*`. Suscríbete cuando el receptor esté preparado y elimina la suscripción en su cierre.

Para una lectura puntual, `GetRadianceRenderTarget()` expone el target actual. El evento comunica la captura del pipeline; un consumidor que necesite datos en CPU debe gestionar la lectura de GPU y su sincronización. No equivale a disponer automáticamente de una copia CPU.

## Adaptador de detector incluido

`IRPipelineActor` obtiene la radiancia del actor asignado, lee el canal R y llama a `ir_process` de `IRPipelineCore`. Presenta la imagen procesada en `ProcessedTexture`.

`IRPipelineConfigAsset` expone parámetros de PSF, calibración radiométrica, retardo térmico, ruido de patrón fijo, ruido de lectura, límite de DN, AGC y mapa de color. Si no se asigna un asset, el adaptador usa la configuración por defecto de la biblioteca.

Esta capa pertenece al proyecto de demostración. Para trasladarla a otro proyecto deben integrarse sus fuentes y la dependencia `IRPipelineCore`; copiar únicamente `IRSimPlugin` no traslada el adaptador.

## Comprobaciones

Para el núcleo, desde la raíz:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Para Unreal, abre **Tools → Test Automation**, filtra `IRSimClean.Radiance` y ejecuta los cinco casos. La prueba carga `/Game/Maps/IRBufferValidation`.

Los scripts de validación y rendimiento heredados contienen rutas del workspace del TFM. Hasta hacerlos portables, usa el editor para la suite; no ejecutes esos scripts suponiendo que ya aceptan la estructura de cualquier clon.

## Límites actuales

La radiometría simplifica cielo y atmósfera. Los parámetros de demostración no constituyen una calibración frente a una cámara concreta. La integración del detector demuestra la cadena, pero requiere ampliar la validación conjunta y documentar el coste de lectura GPU/CPU. Las tareas de continuación están en [ROADMAP.md](ROADMAP.md).
