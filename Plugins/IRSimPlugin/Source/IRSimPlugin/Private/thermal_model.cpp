#include "thermal_model.h"

#include <algorithm>
#include <cmath>

// Implementacion de las ecuaciones radiometricas usadas por el plugin
// Se mantiene separada de los actores para poder probarla fuera de Unreal

namespace ir {

namespace {

constexpr double MICRONS_TO_METERS = 1.0e-6;

double clamp_temperature(double temperature_k) noexcept {
    return std::max(temperature_k, 1.0);
}

} // namespace

double compute_planck_spectral_radiance(double temperature_k, double wavelength_um) noexcept {
	// La longitud de onda llega en micras y se convierte a metros para Planck
    const double lambda_m = std::max(wavelength_um, 1.0e-6) * MICRONS_TO_METERS;
    const double temperature = clamp_temperature(temperature_k);
    const double numerator = 2.0 * PLANCK * SPEED_OF_LIGHT * SPEED_OF_LIGHT;
    const double denominator_lambda = std::pow(lambda_m, 5.0);
    const double exponent = (PLANCK * SPEED_OF_LIGHT) / (lambda_m * BOLTZMANN * temperature);
    const double denominator_exp = std::exp(exponent) - 1.0;
    return numerator / (denominator_lambda * std::max(denominator_exp, 1.0e-12));
}

double integrate_band_radiance(
    double temperature_k,
    double emissivity,
    const SpectralBand& band) noexcept {
	// La suma recorre la banda espectral y aproxima la integral con trapecios
    const int samples = std::max(band.integration_samples, 2);
    const double lambda_low = std::min(band.wavelength_low_um, band.wavelength_high_um);
    const double lambda_high = std::max(band.wavelength_low_um, band.wavelength_high_um);
    const double delta_um = (lambda_high - lambda_low) / static_cast<double>(samples - 1);
    const double delta_m = delta_um * MICRONS_TO_METERS;

    double integral = 0.0;
    for (int i = 0; i < samples; ++i) {
        const double lambda_um = lambda_low + static_cast<double>(i) * delta_um;
        const double weight = (i == 0 || i == samples - 1) ? 0.5 : 1.0;
        integral += weight * compute_planck_spectral_radiance(temperature_k, lambda_um);
    }

    return emissivity * integral * delta_m;
}

double compute_atmospheric_transmittance(double extinction_coefficient, double distance_m) noexcept {
    return std::clamp(std::exp(-std::max(extinction_coefficient, 0.0) * std::max(distance_m, 0.0)), 0.0, 1.0);
}

double compute_surface_band_radiance(
    double object_temperature_k,
    double background_temperature_k,
    double emissivity,
    double transmissivity,
    const SpectralBand& band) noexcept {
	// La superficie se modela como cuerpo gris con una contribucion reflejada
    const double clamped_emissivity = std::clamp(emissivity, 0.0, 1.0);
    const double clamped_transmissivity = std::clamp(transmissivity, 0.0, 1.0);
    const double reflectivity = std::max(0.0, 1.0 - clamped_emissivity - clamped_transmissivity);
    const double emitted = integrate_band_radiance(object_temperature_k, clamped_emissivity, band);
    const double reflected = integrate_band_radiance(background_temperature_k, reflectivity, band);
    return emitted + reflected;
}

double compute_sensor_band_radiance(
    double surface_band_radiance,
    double air_band_radiance,
    double atmospheric_transmittance) noexcept {
    const double tau = std::clamp(atmospheric_transmittance, 0.0, 1.0);
    return tau * surface_band_radiance + (1.0 - tau) * air_band_radiance;
}

float radiance_to_intensity(double radiance, double max_radiance) noexcept {
    if (max_radiance <= 0.0) {
        return 0.0f;
    }

    const double normalized = radiance / max_radiance;
    return static_cast<float>(std::clamp(normalized, 0.0, 1.0));
}

float radiance_to_windowed_intensity(
    double radiance,
    double min_radiance,
    double max_radiance) noexcept {
    if (max_radiance <= min_radiance) {
        return 0.0f;
    }

    const double normalized = (radiance - min_radiance) / (max_radiance - min_radiance);
    return static_cast<float>(std::clamp(normalized, 0.0, 1.0));
}

} // namespace ir
