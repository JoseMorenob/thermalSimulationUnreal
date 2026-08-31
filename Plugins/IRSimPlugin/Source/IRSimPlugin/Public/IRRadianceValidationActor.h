// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// Validacion temporalmente desactivada mientras se consolida el pipeline principal
/*

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IRRadianceValidationActor.generated.h"

// Actor que muestra el error entre la referencia CPU y la salida GPU

class ARadianceCaptureActor;
class UIRThermalSurfaceComponent;

UCLASS(Blueprintable)
class IRSIMPLUGIN_API AIRRadianceValidationActor : public AActor
{
	GENERATED_BODY()

public:
	AIRRadianceValidationActor();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "IR Validation")
	void RunValidation();

	void ConfigureSurfaceValidation(UIRThermalSurfaceComponent* InTarget, ARadianceCaptureActor* InCapture);
	bool HasValidationCompleted() const { return bValidationCompleted; }
	bool HasValidationPassed() const { return bValidationPassed; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation")
	TObjectPtr<UIRThermalSurfaceComponent> TargetThermalSurface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation")
	TObjectPtr<ARadianceCaptureActor> RadianceCaptureActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation", meta = (ClampMin = "0"))
	int32 SampleX = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation", meta = (ClampMin = "0"))
	int32 SampleY = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation")
	bool bAutoSampleTargetCenter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Validation", meta = (ClampMin = "0.0"))
	float MaxRelativeErrorPercent = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Result")
	float ExpectedSensorRadiance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Result")
	float RenderTargetRadiance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Result")
	float AbsoluteError = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Result")
	float RelativeErrorPercent = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Diagnostics")
	float MaxRenderTargetRadiance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Diagnostics")
	int32 MaxRadianceX = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Diagnostics")
	int32 MaxRadianceY = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Diagnostics")
	int32 NonZeroPixelCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Result")
	bool bValidationCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Validation|Result")
	bool bValidationPassed = false;

private:
	bool ReadRenderTargetRadiance(float& OutRadiance);
	bool ResolveTarget(FVector& OutWorldLocation, float& OutExpectedRadiance);
};

*/
