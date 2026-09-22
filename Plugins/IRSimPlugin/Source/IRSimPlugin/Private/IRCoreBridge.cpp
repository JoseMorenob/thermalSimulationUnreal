// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRCoreBridge.h"

#include "thermal_model.h" // Canonical core header installed by build_ir_core.ps1

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

	float ComputeBandRadiance(
		float TemperatureK,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount)
	{
		return static_cast<float>(ir::integrate_band_radiance(
			TemperatureK,
			MakeBand(BandMinMicrons, BandMaxMicrons, SampleCount)));
	}

	float ComputeAtmosphericTransmittance(float ExtinctionCoefficient, float DistanceMeters)
	{
		return static_cast<float>(ir::compute_atmospheric_transmittance(ExtinctionCoefficient, DistanceMeters));
	}

	float ComputeFresnelConductorReflectance(
		float CosThetaI,
		float RefractiveIndexReal,
		float RefractiveIndexImaginary)
	{
		return static_cast<float>(ir::compute_fresnel_conductor_reflectance(
			CosThetaI,
			RefractiveIndexReal,
			RefractiveIndexImaginary));
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

}
