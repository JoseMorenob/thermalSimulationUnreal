// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThermalPipelineController.generated.h"

class ARadianceCaptureActor;
class AThermalCubeActor;
class UMaterialInterface;

UCLASS(Blueprintable)
class IRSIMCLEAN_API AThermalPipelineController : public AActor
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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	TObjectPtr<ARadianceCaptureActor> RadianceCaptureActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	TObjectPtr<UMaterialInterface> DefaultThermalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	bool bAutoAssignMaterialToThermalActors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Pipeline")
	bool bCaptureAfterRefresh = true;

private:
	void UpdateThermalActorsSensorLocation();
	void ApplyThermalMaterialToActors();
	void RefreshCaptureActor();
};
