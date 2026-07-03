// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadianceCaptureActor.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class IRSIMCLEAN_API ARadianceCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ARadianceCaptureActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Radiance")
	void CaptureRadianceFrame();

	UFUNCTION(BlueprintPure, Category = "Radiance")
	UTextureRenderTarget2D* GetRadianceRenderTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Radiance")
	void RefreshCapturePipeline();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "16", UIMin = "16"))
	int32 TargetWidth = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "16", UIMin = "16"))
	int32 TargetHeight = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance")
	bool bCaptureEveryFrame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance")
	bool bCaptureOnMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance")
	bool bUseSingleChannelRenderTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance")
	TObjectPtr<UMaterialInterface> PostProcessThermalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
	float RadianceNormalizationMax = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "0.1"))
	float BandMinMicrons = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "0.1"))
	float BandMaxMicrons = 12.0f;

private:
	void EnsureRenderTarget();

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RadianceRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicPostProcessMaterial;
};
