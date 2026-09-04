// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRCoreBridge.h"

#include "thermal_model.h" // Canonical core header installed by build_ir_core.ps1

#include "Math/UnrealMathUtility.h"

// Adaptador que traduce las llamadas de Unreal a la API del nucleo fisico
// No anade logica radiometrica propia

namespace irsim::core
{
	namespace
	{
		ir::SpectralBand MakeBand(float BandMinMicrons, float BandMaxMicrons, int SampleCount)
		{
			return ir::SpectralBand {
				BandMinMicrons,
				BandMaxMicrons,
				SampleCount
			};
		}
	}

	float ComputePlanckSpectralRadiance(float TemperatureK, float WavelengthMicrons)
	{
		return static_cast<float>(ir::compute_planck_spectral_radiance(TemperatureK, WavelengthMicrons));
	}

	float ComputeBandRadiance(
		float TemperatureK,
		float Emissivity,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount)
	{
		return static_cast<float>(ir::integrate_band_radiance(
			TemperatureK,
			Emissivity,
			MakeBand(BandMinMicrons, BandMaxMicrons, SampleCount)));
	}

	float ComputeAtmosphericTransmittance(float ExtinctionCoefficient, float DistanceMeters)
	{
		return static_cast<float>(ir::compute_atmospheric_transmittance(ExtinctionCoefficient, DistanceMeters));
	}

	float ComputeSurfaceBandRadiance(
		float ObjectTemperatureK,
		float BackgroundTemperatureK,
		float Emissivity,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount)
	{
		return static_cast<float>(ir::compute_surface_band_radiance(
			ObjectTemperatureK,
			BackgroundTemperatureK,
			Emissivity,
			MakeBand(BandMinMicrons, BandMaxMicrons, SampleCount)));
	}

	float ComputeSensorBandRadiance(
		float SurfaceRadiance,
		float AirRadiance,
		float AtmosphericTransmittance)
	{
		return static_cast<float>(ir::compute_sensor_band_radiance(
			SurfaceRadiance,
			AirRadiance,
			AtmosphericTransmittance));
	}

	float ComputeThermalTemperatureStep(
		float ObjectTemperatureK,
		float SolarIrradianceWm2,
		float SolarAbsorptivity,
		float SunExposure,
		float ConvectionCoefficientWm2K,
		float AirTemperatureK,
		float SkyTemperatureK,
		float Emissivity,
		float ThermalCapacityJm2K,
		float DeltaTimeSeconds)
	{
		// Fallback keeps the current prebuilt core ABI usable until ir_core.lib is rebuilt.
		const float temperature = FMath::Max(ObjectTemperatureK, 1.0f);
		const float solarFlux = FMath::Clamp(SolarAbsorptivity, 0.0f, 1.0f) *
			FMath::Max(SolarIrradianceWm2, 0.0f) * FMath::Clamp(SunExposure, 0.0f, 1.0f);
		const float convectionFlux = FMath::Max(ConvectionCoefficientWm2K, 0.0f) * (AirTemperatureK - temperature);
		const float radiationFlux = FMath::Clamp(Emissivity, 0.0f, 1.0f) * 5.670374419e-8f *
			(FMath::Pow(FMath::Max(SkyTemperatureK, 1.0f), 4.0f) - FMath::Pow(temperature, 4.0f));
		const float capacity = FMath::Max(ThermalCapacityJm2K, 1.0e-6f);
		const float deltaTime = FMath::Clamp(DeltaTimeSeconds, 0.0f, 10.0f);
		return FMath::Max(1.0f, temperature + deltaTime * (solarFlux + convectionFlux + radiationFlux) / capacity);
	}

	float RadianceToIntensity(float Radiance, float MaxRadiance)
	{
		return ir::radiance_to_intensity(Radiance, MaxRadiance);
	}

	float RadianceToWindowedIntensity(float Radiance, float MinRadiance, float MaxRadiance)
	{
		return ir::radiance_to_windowed_intensity(Radiance, MinRadiance, MaxRadiance);
	}
}
