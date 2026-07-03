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
    double emissivity,
    const SpectralBand& band) noexcept;

double compute_atmospheric_transmittance(double extinction_coefficient, double distance_m) noexcept;

double compute_surface_band_radiance(
    double object_temperature_k,
    double background_temperature_k,
    double emissivity,
    double transmissivity,
    const SpectralBand& band) noexcept;

double compute_sensor_band_radiance(
    double surface_band_radiance,
    double air_band_radiance,
    double atmospheric_transmittance) noexcept;

float radiance_to_intensity(double radiance, double max_radiance) noexcept;

} // namespace ir

