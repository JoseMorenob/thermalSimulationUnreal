// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IRThermalSurfaceComponent.generated.h"

// Componente que conecta una malla de Unreal con el modelo termico del plugin

class AIRSceneEnvironmentActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(ClassGroup = (IRSim), Blueprintable, meta = (BlueprintSpawnableComponent))
class IRSIMPLUGIN_API UIRThermalSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIRThermalSurfaceComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	void AdvanceThermalState(float DeltaTimeSeconds, bool bSceneDynamicsEnabled,
		float SolarIrradianceWm2, float AirTemperatureK, float SkyTemperatureK);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void RefreshThermalSurface();

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void ApplySceneEnvironment(const AIRSceneEnvironmentActor* SceneEnvironment);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetRadianceSensorWorldLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetTemperatureKelvin(float InTemperatureK);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetEmissivity(float InEmissivity);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Environment")
	void SetAtmosphericExtinctionCoefficient(float InCoefficient);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetDebugMaterial(UMaterialInterface* InDebugMaterial);

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	UStaticMeshComponent* GetTargetMesh() const { return ResolveTargetMesh(); }

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetCurrentSurfaceBandRadiance() const;

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetCurrentAirBandRadiance() const;

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetCurrentSensorRadiance(float DistanceMeters) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface")
	TObjectPtr<UMaterialInterface> DebugMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface", meta = (ClampMin = "0.0"))
	float TemperatureK = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Dynamics")
	bool bEnableThermalDynamics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Dynamics", meta = (ClampMin = "0.0"))
	float ThermalCapacityJm2K = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Dynamics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SolarAbsorptivity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Dynamics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SunExposure = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Dynamics", meta = (ClampMin = "0.0"))
	float ConvectionCoefficientWm2K = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Emissivity = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface", meta = (ClampMin = "0"))
	int32 MaterialId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Directional Emissivity")
	bool bUseDirectionalEmissivity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Directional Emissivity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectionalEmissivityFront = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Directional Emissivity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectionalEmissivityGrazing = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Directional Emissivity", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DirectionalAngularFalloffPower = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Debug Display", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
	float RadianceNormalizationMax = 1000.0f;

private:
	UStaticMeshComponent* ResolveTargetMesh() const;
	float GetSensorDistanceMeters() const;
	void PushThermalDataToPrimitive();

	float AirTemperatureK = 293.15f;
	float EffectiveSkyTemperatureK = 240.0f;
	float AtmosphericExtinctionCoefficient = 0.015f;
	float BandMinMicrons = 8.0f;
	float BandMaxMicrons = 12.0f;
	int32 SpectralIntegrationSamples = 40;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicDebugMaterial;

	bool bHasRadianceSensorWorldLocation = false;
	FVector RadianceSensorWorldLocation = FVector::ZeroVector;
};
