# Balance térmico

La variabilidad térmica del componente usa un modelo reducido de respuesta de primer orden hacia una temperatura objetivo compuesta. No hay perfiles artificiales de tipo escalón, seno o ciclo.

## Configuración

En `IR Thermal Surface`:

- `Use Thermal Balance`: activa la evolución temporal.
- `Temperature K`: temperatura actual/inicial del objeto.
- `Thermal Reference Temperature K`: temperatura de referencia desde la que se expresan las contribuciones ambientales.
- `Solar Temperature Contribution K`: contribución simplificada del Sol; puede ser positiva o negativa para representar calentamiento o corrección radiativa.
- `Air Temperature Influence`: peso de la diferencia entre la temperatura del aire y la referencia. Puede producir calentamiento o enfriamiento.
- `Sky Temperature Influence`: peso de la diferencia entre la temperatura radiativa del cielo y la referencia. Normalmente puede producir enfriamiento.
- `Thermal Time Constant Seconds`: rapidez de calentamiento/enfriamiento.

La temperatura objetivo se calcula como:

`T_target = T_reference + DeltaT_solar + k_air (T_air - T_reference) + k_sky (T_sky - T_reference)`

Las contribuciones pueden ser positivas o negativas. La temperatura del objeto se aproxima progresivamente a ese objetivo y después permanece estable.

La casilla `Use Thermal Balance` controla el `Tick` del componente. Si está desactivada, `Temperature K` permanece fija; si está activada, `TickComponent` llama a `UpdateThermalBalance` en cada frame. También puede cambiarse en Blueprint mediante `SetUseThermalBalance`.

La actualización sigue una respuesta exponencial de primer orden inspirada en el modelo de capacidad concentrada de Incropera et al. (2011), *Fundamentals of Heat and Mass Transfer*. Es una aproximación de control térmico, no un balance energético completo: todavía no resuelve conducción interna, convección espacial, masa, área, calor específico ni irradiancia solar en `W/m²`.

Para un coche de prueba se recomienda comenzar con `Temperature K = 300 K`, referencia `293.15 K`, contribución solar `10 K`, influencia del aire `1.0`, influencia del cielo `0.0` y constante `120 s`. Para enfriamiento nocturno se puede usar contribución solar `0 K` y una influencia del cielo negativa, con una constante de `180–300 s`.
