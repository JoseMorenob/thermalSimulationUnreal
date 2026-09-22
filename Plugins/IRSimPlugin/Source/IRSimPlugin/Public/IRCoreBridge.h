// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// Funciones de acceso al nucleo fisico desde las clases de Unreal

namespace irsim::core
{
	float ComputeBandRadiance(
		float TemperatureK,
		float BandMinMicrons,
		float BandMaxMicrons,
		int SampleCount);

	float ComputeAtmosphericTransmittance(float ExtinctionCoefficient, float DistanceMeters);

	float ComputeFresnelConductorReflectance(
		float CosThetaI,
		float RefractiveIndexReal,
		float RefractiveIndexImaginary);

	float ComputeSensorBandRadiance(
		float SurfaceRadiance,
		float AirRadiance,
		float AtmosphericTransmittance);

}
