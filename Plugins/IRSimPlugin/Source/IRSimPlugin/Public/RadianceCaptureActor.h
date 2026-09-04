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
class UIRThermalSurfaceComponent;

UENUM(BlueprintType)
enum class EIRDebugBuffer : uint8
{
	Radiance UMETA(DisplayName = "Radiance"),
	Temperature UMETA(DisplayName = "Temperature"),
	Emissivity UMETA(DisplayName = "Emissivity"),
	Depth UMETA(DisplayName = "Depth"),
	Normals UMETA(DisplayName = "Normals"),
	MaterialId UMETA(DisplayName = "Material ID")
};

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

	UFUNCTION(BlueprintPure, Category = "Radiance|Buffers")
	UTextureRenderTarget2D* GetTemperatureRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Radiance|Buffers")
	UTextureRenderTarget2D* GetEmissivityRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Radiance|Buffers")
	UTextureRenderTarget2D* GetDepthRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Radiance|Buffers")
	UTextureRenderTarget2D* GetNormalRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Radiance|Buffers")
	UTextureRenderTarget2D* GetMaterialIdRenderTarget() const;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance|Buffers", Transient)
	TObjectPtr<UTextureRenderTarget2D> TemperatureRenderTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance|Buffers", Transient)
	TObjectPtr<UTextureRenderTarget2D> EmissivityRenderTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance|Buffers", Transient)
	TObjectPtr<UTextureRenderTarget2D> DepthRenderTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance|Buffers", Transient)
	TObjectPtr<UTextureRenderTarget2D> NormalRenderTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radiance|Buffers", Transient)
	TObjectPtr<UTextureRenderTarget2D> MaterialIdRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Buffers")
	TObjectPtr<UMaterialInterface> TemperatureBufferMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Buffers")
	TObjectPtr<UMaterialInterface> EmissivityBufferMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Buffers")
	TObjectPtr<UMaterialInterface> MaterialIdBufferMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Buffers")
	bool bCaptureAuxiliaryBuffers = true;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display")
	EIRDebugBuffer DebugBuffer = EIRDebugBuffer::Radiance;

private:
	void EnsureRenderTarget();
	void CaptureAuxiliaryBuffers();
	void CaptureSceneToTarget(UTextureRenderTarget2D* Target, ESceneCaptureSource Source);
	void CaptureThermalMaterialToTarget(UTextureRenderTarget2D* Target, UMaterialInterface* BufferMaterial);
	void SyncToPlayerCamera();
	void UpdatePlayerCameraView();
	UTextureRenderTarget2D* GetDebugRenderTarget() const;
	void ClearPlayerCameraView();
	UCameraComponent* FindPlayerCameraComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicPlayerViewMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> BoundPlayerCameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> AuxiliaryCaptureComponent;
};
