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

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	float GetAirTemperatureK() const { return AirTemperatureK; }
	float GetSkyHorizonTemperatureK() const { return SkyHorizonTemperatureK; }
	float GetSkyZenithTemperatureK() const { return SkyZenithTemperatureK; }
	float GetAtmosphericExtinctionCoefficient() const { return AtmosphericExtinctionCoefficient; }
	float GetBandMinMicrons() const { return BandMinMicrons; }
	float GetBandMaxMicrons() const { return BandMaxMicrons; }
	int32 GetSpectralIntegrationSamples() const { return SpectralIntegrationSamples; }

	UFUNCTION(BlueprintCallable, Category = "IR Environment")
	void SetAirTemperatureKelvin(float InTemperatureK);

	UFUNCTION(BlueprintCallable, Category = "IR Environment")
	void SetAtmosphericExtinctionCoefficient(float InCoefficient);

	UFUNCTION(BlueprintCallable, Category = "IR Environment|Sky Reflection")
	void SetSkyTemperaturesKelvin(float InHorizonK, float InZenithK);

	UFUNCTION(BlueprintCallable, Category = "IR Sensor Band")
	void SetSpectralBand(float InBandMinMicrons, float InBandMaxMicrons, int32 InIntegrationSamples);

private:
	// El entorno es el origen de los parametros compartidos. Los aplica de
	// forma directa para que la atenuacion no dependa de una referencia manual
	// del controlador de la escena.
	void ApplyToThermalSurfaces();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment")
	float AirTemperatureK = 293.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment|Sky Reflection", meta = (ClampMin = "0.0"))
	float SkyHorizonTemperatureK = 275.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment|Sky Reflection", meta = (ClampMin = "0.0"))
	float SkyZenithTemperatureK = 230.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Environment", meta = (ClampMin = "0.0"))
	float AtmosphericExtinctionCoefficient = 0.015f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Sensor Band", meta = (ClampMin = "0.1"))
	float BandMinMicrons = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Sensor Band", meta = (ClampMin = "0.1"))
	float BandMaxMicrons = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Sensor Band", meta = (ClampMin = "4", UIMin = "4"))
	int32 SpectralIntegrationSamples = 40;
};
