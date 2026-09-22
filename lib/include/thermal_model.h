#pragma once

#include "ir_constants.h"

namespace ir {

// Banda espectral de integración. Las longitudes de onda se expresan en µm.
struct SpectralBand {
    double wavelength_low_um;
    double wavelength_high_um;
    int integration_samples;
};

// Radiancia espectral de cuerpo negro, en W m^-3 sr^-1.
double compute_planck_spectral_radiance(
    double temperature_k,
    double wavelength_um) noexcept;

// Radiancia integrada sobre una banda, en W m^-2 sr^-1.
double integrate_band_radiance(
    double temperature_k,
    const SpectralBand& band) noexcept;

// Transmitancia de Beer-Lambert para un coeficiente de extinción en m^-1.
double compute_atmospheric_transmittance(
    double extinction_coefficient,
    double distance_m) noexcept;

// Reflectancia Fresnel no polarizada de un conductor con índice complejo n + iκ.
double compute_fresnel_conductor_reflectance(
    double cos_theta_i,
    double refractive_index_real,
    double refractive_index_imaginary) noexcept;

// Radiancia que alcanza el sensor tras sumar emisión de superficie y aire.
double compute_sensor_band_radiance(
    double surface_band_radiance,
    double air_band_radiance,
    double atmospheric_transmittance) noexcept;

} // namespace ir
