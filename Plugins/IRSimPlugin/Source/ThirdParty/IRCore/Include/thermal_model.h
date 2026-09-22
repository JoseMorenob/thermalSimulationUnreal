#pragma once

#include "ir_constants.h"

namespace ir {

struct SpectralBand {
    double wavelength_low_um;
    double wavelength_high_um;
    int integration_samples;
};

double compute_planck_spectral_radiance(double temperature_k, double wavelength_um) noexcept;

double integrate_band_radiance(
    double temperature_k,
    const SpectralBand& band) noexcept;

double compute_atmospheric_transmittance(double extinction_coefficient, double distance_m) noexcept;

// Unpolarized Fresnel reflectance for an air-to-conductor interface.
// Formula: Pharr, Jakob and Humphreys (2023), Physically Based Rendering,
// 4th ed., section "Specular Reflection and Transmission".
double compute_fresnel_conductor_reflectance(
    double cos_theta_i,
    double refractive_index_real,
    double refractive_index_imaginary) noexcept;

double compute_sensor_band_radiance(
    double surface_band_radiance,
    double air_band_radiance,
    double atmospheric_transmittance) noexcept;

} // namespace ir
