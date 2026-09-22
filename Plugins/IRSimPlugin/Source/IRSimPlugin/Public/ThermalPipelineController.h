// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThermalPipelineController.generated.h"

// Controlador que mantiene conectados entorno objetos y sensor virtual

class ARadianceCaptureActor;
class AIRSceneEnvironmentActor;
class UMaterialInterface;

UCLASS(Blueprintable)
class IRSIMPLUGIN_API AThermalPipelineController : public AActor
{
	GENERATED_BODY()

public:
	AThermalPipelineController();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Thermal Pipeline")
	void RefreshPipeline();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Thermal Pipeline")
	void CaptureRadianceNow();

	// Permite a los componentes cargados posteriormente por World Partition
	// recuperar el mismo material térmico que aplica el controlador.
	UMaterialInterface* GetDefaultThermalMaterial() const { return DefaultThermalMaterial; }
	bool ShouldAutoAssignThermalMaterial() const { return bAutoAssignMaterialToThermalActors; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	TObjectPtr<ARadianceCaptureActor> RadianceCaptureActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	TObjectPtr<AIRSceneEnvironmentActor> SceneEnvironmentActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	TObjectPtr<UMaterialInterface> DefaultThermalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	bool bAutoAssignMaterialToThermalActors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	bool bCaptureAfterRefresh = true;

private:
	void ApplySceneEnvironmentToActors();
	void ApplyThermalMaterialToActors();
	void RefreshCaptureActor();
};
