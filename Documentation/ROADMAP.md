# Continuación del proyecto tras el TFM

El TFM está terminado. La siguiente etapa desarrolla el proyecto como una integración reutilizable de radiometría, motor y procesamiento del detector. Este documento describe trabajo previsto; no atribuye estas capacidades a la versión actual.

## Hito 1 — Presentación pública

- README principal en inglés con portada, animación y vídeo de capturas reales.
- Perfil de autor coherente con C++, aviónica y simulación.
- Guía técnica en español con rutas, dependencias y contrato del buffer.

**Criterio de aceptación:** desde GitHub se puede entender el propósito, ver la demostración y localizar cómo ejecutar el proyecto y conectar el plugin.

## Hito 2 — Integración reproducible

- Hacer portables los scripts de compilación, validación y rendimiento.
- Documentar compatibilidad y distribución de las dos bibliotecas independientes.
- Crear una escena mínima de integración con referencias explícitas.
- Definir cuándo un frame físico está disponible y cómo se sincroniza su consumidor.
- Asegurar desuscripción y limpieza al detener Play, cambiar mapa o retirar un actor.

**Criterio de aceptación:** un clon limpio en el entorno soportado compila, ejecuta la escena mínima y alimenta al detector siguiendo una única guía.

## Hito 3 — Validación conjunta y rendimiento

- Ampliar casos CPU/GPU a distancias, ángulos y geometrías no planas.
- Comprobar continuidad de dimensiones, unidades y configuración espectral entre etapas.
- Medir por separado captura, lectura GPU/CPU, procesamiento y presentación.
- Evaluar lectura asíncrona y recursos de GPU solo cuando las mediciones lo justifiquen.
- Conservar resultados reproducibles con hardware, mapa, resolución y configuración.

**Criterio de aceptación:** cada etapa tiene límites de aceptación y una medida repetible de coste; los resultados distinguen radiometría de detector.

## Hito 4 — Capturas RGB/IR y exportación

- Mantener una ruta visible RGB que preserve los materiales originales.
- Sincronizar cámaras, pose, resolución e identificador de frame.
- Exportar radiancia física e imagen procesada junto con metadatos de banda, entorno y configuración.
- Añadir profundidad, normales e identificadores cuando el consumidor los necesite.

**Criterio de aceptación:** un escenario genera pares RGB/IR sincronizados y datos suficientes para reproducir la captura. La exportación automática de datasets permanece planificada.

## Hito 5 — Modelos ampliados

Evaluar respuesta espectral medida, propiedades materiales con procedencia, atmósfera más detallada y dinámica térmica. Cada ampliación debe incluir referencia física, alcance explícito y validación antes de incorporarse a la ruta principal.

**Criterio de aceptación:** las nuevas opciones explican su validez y mantienen el núcleo independiente del motor.
