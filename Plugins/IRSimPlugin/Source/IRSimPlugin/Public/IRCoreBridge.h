// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// Funciones de acceso al nucleo fisico desde las clases de Unreal

namespace irsim::core
{
	float ComputePlanckSpectralRadiance(float TemperatureK, float WavelengthMicrons);

	float ComputeBandRadiance(
		float TemperatureK,
		float Emissivity,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount);

	float ComputeAtmosphericTransmittance(float ExtinctionCoefficient, float DistanceMeters);

	float ComputeSurfaceBandRadiance(
		float ObjectTemperatureK,
		float BackgroundTemperatureK,
		float Emissivity,
		float Transmissivity,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount);

	float ComputeSensorBandRadiance(
		float SurfaceRadiance,
		float AirRadiance,
		float AtmosphericTransmittance);

	float RadianceToIntensity(float Radiance, float MaxRadiance);

	float RadianceToWindowedIntensity(
		float Radiance,
		float MinRadiance,
		float MaxRadiance);
}
