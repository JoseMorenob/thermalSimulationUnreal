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

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void RefreshThermalSurface();

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void ApplySceneEnvironment(const AIRSceneEnvironmentActor* SceneEnvironment);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetTemperatureKelvin(float InTemperatureK);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetTargetMesh(UStaticMeshComponent* InTargetMesh);

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetTemperatureKelvin() const { return TemperatureK; }

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface|Optical Parameters")
	void SetComplexRefractiveIndex(float InRealPart, float InImaginaryPart);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Environment")
	void SetAtmosphericExtinctionCoefficient(float InCoefficient);

	UFUNCTION(BlueprintCallable, Category = "IR Thermal Surface")
	void SetDebugMaterial(UMaterialInterface* InDebugMaterial);

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	UStaticMeshComponent* GetTargetMesh() const { return ResolveTargetMesh(); }

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetCurrentBlackbodyBandRadiance() const;

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetCurrentAirBandRadiance() const;

	UFUNCTION(BlueprintPure, Category = "IR Thermal Surface")
	float GetCurrentBlackbodySensorRadiance(float DistanceMeters) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface")
	TObjectPtr<UMaterialInterface> DebugMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface", meta = (ClampMin = "0.0"))
	float TemperatureK = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface", meta = (ClampMin = "0"))
	int32 MaterialId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Optical Parameters", meta = (ClampMin = "0.0"))
	float ComplexRefractiveIndexReal = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Thermal Surface|Optical Parameters", meta = (ClampMin = "0.0"))
	float ComplexRefractiveIndexImaginary = 0.0f;

private:
	UStaticMeshComponent* ResolveTargetMesh() const;
	void PushThermalDataToPrimitive();

	float AirTemperatureK = 293.15f;
	float SkyHorizonTemperatureK = 275.0f;
	float SkyZenithTemperatureK = 230.0f;
	float AtmosphericExtinctionCoefficient = 0.015f;
	float BandMinMicrons = 8.0f;
	float BandMaxMicrons = 14.0f;
	int32 SpectralIntegrationSamples = 40;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicDebugMaterial;

};
