#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "thermal_model.h"

namespace {

constexpr double RELATIVE_TOLERANCE = 0.01;

struct TestContext {
    int failed = 0;
};

void expect_true(TestContext& context, bool condition, const std::string& test_name)
{
    if (!condition)
    {
        ++context.failed;
        std::cerr << "FAILED: " << test_name << '\n';
    }
}

void expect_near(
    TestContext& context,
    double actual,
    double expected,
    const std::string& test_name)
{
    const double tolerance = std::abs(expected) * RELATIVE_TOLERANCE;
    if (std::abs(actual - expected) > tolerance)
    {
        ++context.failed;
        std::cerr << "FAILED: " << test_name << " (actual=" << actual
                  << ", expected=" << expected << ")\n";
    }
}

void print_result(const std::string& test_name, double actual, double expected)
{
    const double relative_error_percent = std::abs(actual - expected) /
        std::max(std::abs(expected), 1.0e-30) * 100.0;
    std::cout << test_name << ": actual=" << std::fixed << std::setprecision(9)
              << actual << ", expected=" << expected
              << ", relative_error_percent=" << relative_error_percent << '\n';
}

void test_planck_spectral_radiance(TestContext& context)
{
    // NOAA Planck calculator: valores en W m^-2 sr^-1 µm^-1, convertidos a
    // W m^-3 sr^-1 para compararlos con la API del núcleo.
    const double radiance_280_k = ir::compute_planck_spectral_radiance(280.0, 10.0);
    const double radiance_300_k = ir::compute_planck_spectral_radiance(300.0, 10.0);
    const double radiance_320_k = ir::compute_planck_spectral_radiance(320.0, 10.0);

    expect_true(context, radiance_280_k > 0.0, "Planck radiance is positive");
    expect_true(context, radiance_300_k > radiance_280_k,
        "Planck radiance increases from 280 K to 300 K");
    expect_true(context, radiance_320_k > radiance_300_k,
        "Planck radiance increases from 300 K to 320 K");

    constexpr double reference_scale = 1.0e6;
    expect_near(context, radiance_280_k, 7.028585125605226 * reference_scale,
        "Planck radiance at 280 K and 10 um");
    expect_near(context, radiance_300_k, 9.924087014931606 * reference_scale,
        "Planck radiance at 300 K and 10 um");
    expect_near(context, radiance_320_k, 13.431815231907528 * reference_scale,
        "Planck radiance at 320 K and 10 um");

    print_result("Planck_10um_300K", radiance_300_k,
        9.924087014931606 * reference_scale);
}

void test_band_integration(TestContext& context)
{
    // Referencias calculadas con integración de alta resolución de Planck.
    const ir::SpectralBand mwir_band{3.0, 5.0, 40};
    const double mwir_300_k = ir::integrate_band_radiance(300.0, mwir_band);
    const double mwir_320_k = ir::integrate_band_radiance(320.0, mwir_band);
    expect_near(context, mwir_300_k, 1.865956208235,
        "Integrated MWIR radiance at 300 K");
    expect_near(context, mwir_320_k, 3.694727012342,
        "Integrated MWIR radiance at 320 K");
    expect_true(context, mwir_320_k > mwir_300_k,
        "Integrated MWIR radiance increases with temperature");

    const ir::SpectralBand lwir_band{8.0, 14.0, 40};
    const double lwir_300_k = ir::integrate_band_radiance(300.0, lwir_band);
    const double lwir_320_k = ir::integrate_band_radiance(320.0, lwir_band);
    expect_near(context, lwir_300_k, 54.933461376840,
        "Integrated LWIR radiance at 300 K");
    expect_near(context, lwir_320_k, 73.224514739970,
        "Integrated LWIR radiance at 320 K");
    expect_true(context, lwir_320_k > lwir_300_k,
        "Integrated LWIR radiance increases with temperature");

    print_result("LWIR_8_14um_300K", lwir_300_k, 54.933461376840);
}

void test_atmospheric_transmittance(TestContext& context)
{
    const double transmittance = ir::compute_atmospheric_transmittance(0.01, 100.0);
    expect_near(context, transmittance, std::exp(-1.0),
        "Beer-Lambert transmittance at one optical depth");
    expect_near(context, ir::compute_atmospheric_transmittance(-1.0, 50.0), 1.0,
        "Negative extinction is clamped to zero");
}

void test_fresnel_reflectance(TestContext& context)
{
    expect_near(context, ir::compute_fresnel_conductor_reflectance(1.0, 1.0, 0.0),
        0.0, "Matched refractive indices have zero normal reflectance");
    expect_near(context, ir::compute_fresnel_conductor_reflectance(1.0, 1.5, 0.0),
        0.04, "Dielectric normal-incidence Fresnel reflectance");
}

void test_sensor_radiance(TestContext& context)
{
    expect_near(context, ir::compute_sensor_band_radiance(10.0, 2.0, 0.8), 8.4,
        "Sensor radiance combines attenuated surface and air path radiance");
    expect_near(context, ir::compute_sensor_band_radiance(10.0, 2.0, 2.0), 10.0,
        "Sensor radiance clamps transmittance to one");
}

} // namespace

int main()
{
    TestContext context;
    test_planck_spectral_radiance(context);
    test_band_integration(context);
    test_atmospheric_transmittance(context);
    test_fresnel_reflectance(context);
    test_sensor_radiance(context);

    if (context.failed != 0)
    {
        std::cerr << context.failed << " IR core test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All IR core tests passed\n";
    return EXIT_SUCCESS;
}
