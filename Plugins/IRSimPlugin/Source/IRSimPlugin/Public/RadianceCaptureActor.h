// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadianceCaptureActor.generated.h"

// Actor que genera y conserva el render target de radiancia fisica

class USceneCaptureComponent2D;
class UCameraComponent;
class UPostProcessComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class IRSIMPLUGIN_API ARadianceCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ARadianceCaptureActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Radiance")
	void CaptureRadianceFrame();

	UFUNCTION(BlueprintPure, Category = "Radiance")
	UTextureRenderTarget2D* GetRadianceRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Radiance")
	FVector GetSensorWorldLocation() const;

	bool ProjectWorldLocationToRenderTarget(const FVector& WorldLocation, FIntPoint& OutPixel) const;

	UFUNCTION(BlueprintCallable, Category = "Radiance")
	void RefreshCapturePipeline();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance|Player View")
	TObjectPtr<UPostProcessComponent> PlayerViewPostProcessComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "16", UIMin = "16"))
	int32 TargetWidth = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance", meta = (ClampMin = "16", UIMin = "16"))
	int32 TargetHeight = 512;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance", Transient)
	TObjectPtr<UTextureRenderTarget2D> RadianceRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	bool bFollowPlayerCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	bool bShowRenderTargetOnPlayerCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	TObjectPtr<UMaterialInterface> PlayerViewPostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	FName PlayerViewTextureParameterName = TEXT("RadianceTexture");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DisplayRadianceMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
	float DisplayRadianceMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display")
	bool bInvertDebugDisplay = false;

private:
	void EnsureRenderTarget();
	void SyncToPlayerCamera();
	void UpdatePlayerCameraView();
	void ClearPlayerCameraView();
	UCameraComponent* FindPlayerCameraComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicPlayerViewMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> BoundPlayerCameraComponent;
};
