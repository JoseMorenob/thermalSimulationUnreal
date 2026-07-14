# Plan de Emisividad Direccional y Modelo Angular

## Objetivo

Este documento describe:

- como esta formulado el pipeline actual;
- que partes ya estan implementadas;
- que partes faltan para pasar a una emisividad direccional real;
- que formulas habria que cambiar;
- y como quedaria el reparto CPU/GPU en la siguiente iteracion.

La atmosfera espectral avanzada se deja fuera de este plan.

## 1. Estado actual

## 1.1. Hipotesis fisica actual

El modelo actual asume una superficie gris y difusa en banda LWIR:

- emisividad constante por objeto;
- reflectividad simplificada;
- atmosfera simplificada por Beer-Lambert;
- temperatura base por objeto;
- distancia a camara evaluada por pixel.

## 1.2. Formulas actuales

### Emision de superficie

La radiancia de emision en banda se resume como:

`L_emit_band = epsilon * B_band(T_obj)`

donde:

- `epsilon` es constante por objeto;
- `B_band(T_obj)` representa la radiancia integrada en banda derivada de Planck.

### Reflexion simplificada

Se usa:

`rho = 1 - epsilon - tau_mat`

y luego:

`L_refl_band = rho * L_env_band`

En el estado actual `L_env_band` se aproxima mediante una temperatura efectiva de cielo.

### Radiancia de superficie

`L_surface_band = L_emit_band + L_refl_band`

### Atmosfera simplificada

`tau_atm(d) = tau_base * exp(-kappa * d)`

donde:

- `tau_base` es la transmitancia atmosferica base configurable;
- `kappa` es el coeficiente de extincion;
- `d` es la distancia sensor-superficie.

### Radiancia en sensor

`L_sensor_band = tau_atm(d) * L_surface_band + (1 - tau_atm(d)) * L_air_band`

## 1.3. Que parte va en CPU y que parte va en GPU

### CPU por objeto

Se calcula o se prepara:

- `TemperatureK`
- `Emissivity`
- `Transmissivity`
- `AirTemperatureK`
- `EffectiveSkyTemperatureK`
- `AtmosphericTransmittance`
- `AtmosphericExtinctionCoefficient`
- `SurfaceBandRadiance`
- `AirBandRadiance`

### GPU por pixel

Se calcula:

- `d = length(CameraPositionWS - AbsoluteWorldPosition) * 0.01`
- `tau_atm(d)`
- `L_sensor_band`

## 1.4. Que significa esto fisicamente

El pipeline actual ya tiene una parte angular geometrica implicita en el sentido de que:

- cada pixel usa su propia posicion 3D visible;
- cada pixel tiene una distancia distinta a la camara;
- por tanto la atmosfera se evalua por pixel.

Pero todavia no hay una dependencia angular material real del tipo:

- `epsilon(theta_v)`
- `rho(theta_v)`

Eso es lo que falta para tener un modelo angular fisico de superficie.

## 2. Limitacion del modelo actual

El modelo actual detecta bien diferencias debidas a profundidad visible, pero no modela que un mismo material pueda:

- emitir distinto visto de frente o en rasante;
- reflejar mas entorno en angulos rasantes;
- cambiar la proporcion entre `L_emit` y `L_refl` con el angulo de observacion.

Ahora mismo, si dos pixeles del mismo objeto tienen:

- la misma temperatura;
- la misma emisividad;
- la misma atmosfera;

solo cambian por distancia, no por direccion material.

## 3. Estado objetivo de la siguiente iteracion

La siguiente iteracion debe introducir una dependencia angular material en la superficie.

La idea no es multiplicar la radiancia por un `cos(theta)` arbitrario.

La idea correcta es hacer que el angulo modifique:

- la emisividad aparente del material;
- la reflectividad aparente del material;
- y, por tanto, la mezcla entre emision propia y entorno reflejado.

## 4. Formulacion objetivo

## 4.1. Angulo de vista por pixel

Para cada pixel visible:

`V = normalize(CameraPositionWS - AbsoluteWorldPosition)`

`N = normalize(PixelNormalWS)`

`NoV = saturate(dot(N, V))`

`theta_v = arccos(NoV)`

## 4.2. Emisividad direccional

En vez de:

`epsilon = constante`

habria que usar:

`epsilon_dir = epsilon(theta_v)`

o, en una version mas completa:

`epsilon_dir = epsilon(lambda, theta_v)`

Para esta siguiente iteracion no hace falta meter dependencia espectral angular completa. Basta con una ley angular en banda:

`epsilon_dir_band = epsilon_band(theta_v)`

## 4.3. Reflectividad direccional

Si mantenemos la hipotesis de material opaco y gris:

`rho_dir = 1 - epsilon_dir - tau_mat`

Si `tau_mat = 0`, queda:

`rho_dir = 1 - epsilon_dir`

## 4.4. Emision y reflexion con angulo

`L_emit_band = epsilon_dir * B_band(T_obj)`

`L_refl_band = rho_dir * L_env_band`

`L_surface_band = L_emit_band + L_refl_band`

## 4.5. Radiancia en sensor

La atmosfera no cambia en esta iteracion:

`tau_atm(d) = tau_base * exp(-kappa * d)`

`L_sensor_band = tau_atm(d) * L_surface_band + (1 - tau_atm(d)) * L_air_band`

## 5. Que formulas cambian exactamente

## 5.1. Formula actual de superficie

`L_surface_band_actual = epsilon_const * B_band(T_obj) + rho_const * L_env_band`

## 5.2. Formula objetivo de superficie

`L_surface_band_nueva = epsilon(theta_v) * B_band(T_obj) + rho(theta_v) * L_env_band`

Esta es la diferencia principal de la siguiente iteracion.

## 5.3. Atmosfera

No cambia:

`L_sensor_band = tau_atm(d) * L_surface_band + (1 - tau_atm(d)) * L_air_band`

La novedad no esta en la atmosfera, sino en `L_surface_band`.

## 6. Plan de implementacion recomendado

## 6.1. Fase 1: geometria angular por pixel

En GPU/material:

- calcular `V`
- calcular `N`
- calcular `NoV`

Validacion:

- visualizar `NoV` para comprobar que frente y rasante responden como se espera.

## 6.2. Fase 2: modelo direccional minimo

Introducir un modelo simple por material:

- `EmissivityFront`
- `EmissivityGrazing`
- `AngularFalloffPower`

Ejemplo funcional:

`epsilon_dir = lerp(epsilon_grazing, epsilon_front, NoV^p)`

donde:

- `epsilon_front` es la emisividad cerca de vision frontal;
- `epsilon_grazing` es la emisividad cerca de rasante;
- `p` controla la curvatura.

No es una ley universal, pero ya es un modelo direccional explicito y controlable.

## 6.3. Fase 3: reflectividad coherente

Calcular:

`rho_dir = 1 - epsilon_dir - tau_mat`

Y usar:

`L_refl_band = rho_dir * L_env_band`

## 6.4. Fase 4: radiancia final

Sustituir la formula actual por:

`L_emit_band = epsilon_dir * B_band(T_obj)`

`L_refl_band = rho_dir * L_env_band`

`L_surface_band = L_emit_band + L_refl_band`

`L_sensor_band = tau_atm(d) * L_surface_band + (1 - tau_atm(d)) * L_air_band`

## 7. Reparto CPU/GPU en la siguiente iteracion

## 7.1. CPU

Seguiria llevando:

- `TemperatureK`
- `AirTemperatureK`
- `EffectiveSkyTemperatureK`
- `AtmosphericTransmittance`
- `AtmosphericExtinctionCoefficient`
- parametros de material direccional:
  - `EmissivityFront`
  - `EmissivityGrazing`
  - `AngularFalloffPower`
  - `Transmissivity`

La CPU ya no deberia asumir una `SurfaceBandRadiance` final cerrada si esta depende del angulo por pixel.

En esa siguiente iteracion convendria pasar a GPU:

- la radiancia base de Planck integrada en banda o un equivalente escalar preintegrado;
- el entorno radiativo simplificado;
- y los parametros direccionales.

## 7.2. GPU

Deberia calcular por pixel:

- `NoV`
- `epsilon_dir`
- `rho_dir`
- `L_emit_band`
- `L_refl_band`
- `L_surface_band`
- `tau_atm(d)`
- `L_sensor_band`

## 8. Cambio conceptual importante

Con el pipeline actual, el angulo solo afecta de forma indirecta a traves de:

- que pixel es visible;
- que distancia tiene ese pixel a camara.

Con la siguiente iteracion, el angulo pasaria a afectar tambien de forma material:

- cambia `epsilon_dir`;
- cambia `rho_dir`;
- cambia la proporcion emision/reflexion;
- cambia `L_surface_band` incluso aunque la temperatura no cambie.

## 9. Alcance recomendado

Para la siguiente iteracion, lo recomendable es:

1. no tocar atmosfera espectral avanzada;
2. no meter aun respuesta de sensor final;
3. implementar un modelo direccional simple pero explicito;
4. validar con una placa o rampa vista de frente y rasante.

## 10. Resumen corto

Estado actual:

`L_sensor_band = tau_atm(d_pixel) * [epsilon_const * B_band(T_obj) + rho_const * L_env_band] + (1 - tau_atm(d_pixel)) * L_air_band`

Estado objetivo:

`L_sensor_band = tau_atm(d_pixel) * [epsilon(theta_v_pixel) * B_band(T_obj) + rho(theta_v_pixel) * L_env_band] + (1 - tau_atm(d_pixel)) * L_air_band`

La diferencia central de la siguiente iteracion es:

- pasar de `epsilon` constante a `epsilon(theta_v_pixel)`;
- y hacer coherente `rho(theta_v_pixel)` con esa nueva emisividad direccional.
