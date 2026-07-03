// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThermalCubeActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class IRSIMCLEAN_API AThermalCubeActor : public AActor
{
	GENERATED_BODY()

public:
	AThermalCubeActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Thermal")
	float GetTemperatureKelvin() const { return TemperatureK; }

	UFUNCTION(BlueprintPure, Category = "Thermal")
	float GetEmissivity() const { return Emissivity; }

	UFUNCTION(BlueprintPure, Category = "Thermal")
	float GetReflectivity() const { return FMath::Max(0.0f, 1.0f - Emissivity - Transmissivity); }

	UFUNCTION(BlueprintPure, Category = "Thermal")
	float GetObjectDistanceTo(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Thermal")
	float GetCurrentEmittedRadiance() const;

	UFUNCTION(BlueprintPure, Category = "Thermal")
	float GetCurrentSensorRadiance(float DistanceMeters) const;

	UFUNCTION(BlueprintCallable, Category = "Thermal")
	void SetDebugMaterial(UMaterialInterface* InDebugMaterial);

	UFUNCTION(BlueprintCallable, Category = "Thermal")
	void RefreshThermalMaterial();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thermal")
	TObjectPtr<UStaticMeshComponent> CubeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
	TObjectPtr<UMaterialInterface> DebugMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.0"))
	float TemperatureK = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Emissivity = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Transmissivity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
	float AirTemperatureK = 293.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
	float EffectiveSkyTemperatureK = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AtmosphericTransmittance = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.0"))
	float AtmosphericExtinctionCoefficient = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
	float RadianceNormalizationMax = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.1"))
	float BandMinMicrons = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "0.1"))
	float BandMaxMicrons = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal", meta = (ClampMin = "4", UIMin = "4"))
	int32 SpectralIntegrationSamples = 40;

private:
	void PushThermalDataToPrimitive();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicDebugMaterial;
};
