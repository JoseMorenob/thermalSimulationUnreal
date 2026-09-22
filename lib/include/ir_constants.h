#pragma once

namespace ir {

// Constantes SI exactas. Fuente: CODATA 2018, incorporada al SI 2019.
constexpr double PLANCK = 6.62607015e-34;
constexpr double BOLTZMANN = 1.380649e-23;
constexpr double SPEED_OF_LIGHT = 2.99792458e8;
constexpr double STEFAN_BOLTZMANN = 5.670374419e-8;

namespace band {

// Ventana atmosférica LWIR usada por defecto en el proyecto, en micrómetros.
constexpr double LWIR_LOW_UM = 8.0;
constexpr double LWIR_HIGH_UM = 14.0;

} // namespace band

} // namespace ir
