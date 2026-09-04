// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IRSceneEnvironmentActor.generated.h"

// Parametros de aire cielo atmosfera y banda compartidos por la escena

UCLASS(Blueprintable)
class IRSIMPLUGIN_API AIRSceneEnvironmentActor : public AActor
{
	GENERATED_BODY()

public:
	AIRSceneEnvironmentActor();

	float GetAirTemperatureK() const { return AirTemperatureK; }
	float GetEffectiveSkyTemperatureK() const { return EffectiveSkyTemperatureK; }
	float GetSolarIrradianceWm2() const { return SolarIrradianceWm2; }
	bool IsThermalDynamicsEnabled() const { return bEnableThermalDynamics; }
	float GetAtmosphericExtinctionCoefficient() const { return AtmosphericExtinctionCoefficient; }
	float GetBandMinMicrons() const { return BandMinMicrons; }
	float GetBandMaxMicrons() const { return BandMaxMicrons; }
	int32 GetSpectralIntegrationSamples() const { return SpectralIntegrationSamples; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment")
	float AirTemperatureK = 293.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment")
	float EffectiveSkyTemperatureK = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Environment")
	bool bEnableThermalDynamics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Environment", meta = (ClampMin = "0.0"))
	float SolarIrradianceWm2 = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment", meta = (ClampMin = "0.0"))
	float AtmosphericExtinctionCoefficient = 0.015f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Sensor Band", meta = (ClampMin = "0.1"))
	float BandMinMicrons = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Sensor Band", meta = (ClampMin = "0.1"))
	float BandMaxMicrons = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Sensor Band", meta = (ClampMin = "4", UIMin = "4"))
	int32 SpectralIntegrationSamples = 40;
};
