// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRCoreBridge.h"

#include "thermal_model.h"

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
		float Transmissivity,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount)
	{
		return static_cast<float>(ir::compute_surface_band_radiance(
			ObjectTemperatureK,
			BackgroundTemperatureK,
			Emissivity,
			Transmissivity,
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

	float RadianceToIntensity(float Radiance, float MaxRadiance)
	{
		return ir::radiance_to_intensity(Radiance, MaxRadiance);
	}

	float RadianceToWindowedIntensity(float Radiance, float MinRadiance, float MaxRadiance)
	{
		return ir::radiance_to_windowed_intensity(Radiance, MinRadiance, MaxRadiance);
	}
}
