# IR Simulator Plugin

## Panel de control en ejecución

El plugin crea transitoriamente un `IRRuntimeControlActor` al iniciar Play. No
hay que guardarlo en el mapa ni añadirlo a una escena. Para una demostración
personalizada también puede añadirse manualmente: sus propiedades permiten
asignar explícitamente la superficie térmica, el actor de entorno y el
capturador de radiancia. Si se dejan vacíos, el panel usa el primer actor
compatible que encuentre en el mundo.

Durante Play, pulsa `I` para mostrar u ocultar el panel. Permite modificar en
tiempo real la temperatura de la superficie y del aire, la extinción atmosférica,
el rango de visualización de radiancia, la captura continua y el buffer mostrado.
La física permanece en `ir_core`; esta interfaz solo invoca la API del plugin.

## Prueba automatizada

La prueba de radiancia se conserva independiente de las escenas de demostración.
En el editor se ejecuta desde **Tools > Test Automation** con el filtro
`IRSimClean.Radiance`. Carga exclusivamente `/Game/Maps/IRBufferValidation` y
comprueba CPU frente a GPU, atenuación atmosférica, buffers auxiliares y Fresnel.
