#include "thermal_model.h"

#include <algorithm>
#include <cmath>

namespace ir {

namespace {

constexpr double MICRONS_TO_METERS = 1.0e-6;
constexpr double MINIMUM_TEMPERATURE_K = 1.0;
constexpr double MINIMUM_DENOMINATOR = 1.0e-12;

double clamp_temperature(double temperature_k) noexcept
{
    return std::max(temperature_k, MINIMUM_TEMPERATURE_K);
}

} // namespace

double compute_planck_spectral_radiance(
    double temperature_k,
    double wavelength_um) noexcept
{
    // Ley de Planck: L_λ = 2hc² / (λ⁵(exp(hc/(λkT)) - 1)).
    // Fuente: Planck (1901), "On the Law of Distribution of Energy...".
    const double wavelength_m = std::max(wavelength_um, 1.0e-6) * MICRONS_TO_METERS;
    const double temperature_k_clamped = clamp_temperature(temperature_k);
    const double numerator = 2.0 * PLANCK * SPEED_OF_LIGHT * SPEED_OF_LIGHT;
    const double wavelength_to_fifth = std::pow(wavelength_m, 5.0);
    const double exponent = (PLANCK * SPEED_OF_LIGHT) /
        (wavelength_m * BOLTZMANN * temperature_k_clamped);
    const double exponential_term = std::exp(exponent) - 1.0;

    return numerator /
        (wavelength_to_fifth * std::max(exponential_term, MINIMUM_DENOMINATOR));
}

double integrate_band_radiance(
    double temperature_k,
    const SpectralBand& band) noexcept
{
    // Regla trapezoidal de la radiancia de Planck sobre la banda solicitada.
    // Fuente numérica: Press et al. (2007), Numerical Recipes, sec. 4.1.
    const int sample_count = std::max(band.integration_samples, 2);
    const double wavelength_low_um = std::min(
        band.wavelength_low_um, band.wavelength_high_um);
    const double wavelength_high_um = std::max(
        band.wavelength_low_um, band.wavelength_high_um);
    const double delta_um = (wavelength_high_um - wavelength_low_um) /
        static_cast<double>(sample_count - 1);
    const double delta_m = delta_um * MICRONS_TO_METERS;

    double integral = 0.0;
    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        const double wavelength_um = wavelength_low_um +
            static_cast<double>(sample_index) * delta_um;
        const double weight = (sample_index == 0 || sample_index == sample_count - 1)
            ? 0.5
            : 1.0;
        integral += weight * compute_planck_spectral_radiance(temperature_k, wavelength_um);
    }

    return integral * delta_m;
}

double compute_atmospheric_transmittance(
    double extinction_coefficient,
    double distance_m) noexcept
{
    // Ley de Beer-Lambert: τ = exp(-κd).
    // Fuente: Modest (2013), Radiative Heat Transfer, 3.ª ed., cap. 15.
    const double non_negative_extinction = std::max(extinction_coefficient, 0.0);
    const double non_negative_distance = std::max(distance_m, 0.0);
    return std::clamp(
        std::exp(-non_negative_extinction * non_negative_distance), 0.0, 1.0);
}

double compute_fresnel_conductor_reflectance(
    double cos_theta_i,
    double refractive_index_real,
    double refractive_index_imaginary) noexcept
{
    // Reflectancia Fresnel no polarizada de un conductor, con n + iκ.
    // Fuente: Pharr, Jakob y Humphreys (2023), PBRT 4e, sec. 9.3.2.
    const double cos_theta = std::clamp(cos_theta_i, 0.0, 1.0);
    const double eta = std::max(refractive_index_real, 0.0);
    const double kappa = std::max(refractive_index_imaginary, 0.0);
    const double cos_theta_squared = cos_theta * cos_theta;
    const double sin_theta_squared = std::max(1.0 - cos_theta_squared, 0.0);
    const double eta_squared = eta * eta;
    const double kappa_squared = kappa * kappa;
    const double t0 = eta_squared - kappa_squared - sin_theta_squared;
    const double a_squared_plus_b_squared = std::sqrt(
        t0 * t0 + 4.0 * eta_squared * kappa_squared);
    const double a = std::sqrt(std::max(
        0.5 * (a_squared_plus_b_squared + t0), 0.0));
    const double t1 = a_squared_plus_b_squared + cos_theta_squared;
    const double t2 = 2.0 * cos_theta * a;
    const double reflectance_s = (t1 - t2) /
        std::max(t1 + t2, MINIMUM_DENOMINATOR);
    const double t3 = cos_theta_squared * a_squared_plus_b_squared +
        sin_theta_squared * sin_theta_squared;
    const double t4 = t2 * sin_theta_squared;
    const double reflectance_p = reflectance_s * (t3 - t4) /
        std::max(t3 + t4, MINIMUM_DENOMINATOR);

    return std::clamp(0.5 * (reflectance_s + reflectance_p), 0.0, 1.0);
}

double compute_sensor_band_radiance(
    double surface_band_radiance,
    double air_band_radiance,
    double atmospheric_transmittance) noexcept
{
    // Transferencia radiativa sin dispersión: Lsensor = τLsuperficie + (1-τ)Laire.
    // Fuente: Modest (2013), Radiative Heat Transfer, 3.ª ed., cap. 15.
    const double transmittance = std::clamp(atmospheric_transmittance, 0.0, 1.0);
    return transmittance * surface_band_radiance +
        (1.0 - transmittance) * air_band_radiance;
}

} // namespace ir
