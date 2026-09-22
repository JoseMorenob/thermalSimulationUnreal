// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadianceCaptureActor.generated.h"

// Actor que genera y conserva el render target de radiancia fisica

class USceneCaptureComponent2D;
class UCameraComponent;
class UPostProcessComponent;
class UIRInternalSceneCaptureComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UIRThermalSurfaceComponent;
class ACameraActor;

// Evento de integracion: se emite cuando una captura fisica ya esta disponible.
// El receptor debe tratar el target como entrada de solo lectura y crear su
// propio target de salida para ruido, cuantizacion o efectos instrumentales.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnIRRadianceFrameCaptured,
	UTextureRenderTarget2D*, PhysicalRadianceTarget);

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

	// Selects which captured buffer is presented by the optional player-camera
	// debug material. It does not modify any physical render target.
	UFUNCTION(BlueprintCallable, Category = "Radiance|Debug Display")
	void SetDebugBuffer(EIRDebugBuffer InDebugBuffer);

	UFUNCTION(BlueprintPure, Category = "Radiance|Debug Display")
	EIRDebugBuffer GetDebugBuffer() const;

	UFUNCTION(BlueprintCallable, Category = "Radiance")
	void SetRadianceCaptureEnabled(bool bEnabled) { bEnableRadianceCapture = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Radiance")
	bool IsRadianceCaptureEnabled() const { return bEnableRadianceCapture; }

	UFUNCTION(BlueprintCallable, Category = "Radiance|Buffers")
	void SetCaptureAuxiliaryBuffers(bool bEnabled) { bCaptureAuxiliaryBuffers = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Radiance|Debug Display")
	void SetDisplayRadianceRange(float InMin, float InMax);

	UFUNCTION(BlueprintPure, Category = "Radiance|Debug Display")
	float GetDisplayRadianceMax() const { return DisplayRadianceMax; }

	// Se emite una vez terminada cada captura. PhysicalRadianceTarget contiene
	// radiancia LWIR lineal en RGBA16F, con sRGB=false y gamma=1. El canal R es
	// la magnitud fisica W/(m2 sr); no modificar este target desde otro plugin.
	UPROPERTY(BlueprintAssignable, Category = "Radiance|Integration")
	FOnIRRadianceFrameCaptured OnRadianceFrameCaptured;

	UFUNCTION(BlueprintCallable, Category = "Radiance|Buffers")
	void SetAuxiliaryBufferMaterials(
		UMaterialInterface* InTemperatureMaterial,
		UMaterialInterface* InEmissivityMaterial,
		UMaterialInterface* InMaterialIdMaterial);

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
	TObjectPtr<UIRInternalSceneCaptureComponent> CaptureComponent;

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

	// Permite comparar el coste del pipeline IR con una ejecución base de la
	// misma escena sin modificar los actores ni los materiales térmicos.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Benchmark")
	bool bEnableRadianceCapture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	bool bFollowPlayerCamera = false;

	// Referencia explicita para secuencias cinematograficas. Si se asigna,
	// prevalece sobre la camara del Pawn y el capture replica su transform.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	TObjectPtr<ACameraActor> FollowCameraActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	bool bShowRenderTargetOnPlayerCamera = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	TObjectPtr<UMaterialInterface> PlayerViewPostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Player View")
	FName PlayerViewTextureParameterName = TEXT("RadianceTexture");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DisplayRadianceMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
	float DisplayRadianceMax = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float DebugDepthMaxCentimeters = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float DebugMaterialIdMax = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display")
	EIRDebugBuffer DebugBuffer = EIRDebugBuffer::Radiance;

	// Runtime shortcuts: 1 radiance, 2 temperature, 3 emissivity, 4 depth,
	// 5 normals and 6 material ID. They are active only during Play.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiance|Debug Display")
	bool bEnableDebugBufferHotkeys = true;

private:
	// The command-line benchmark mode is deliberately transient: it never
	// changes the level asset.  See Scripts/run_ir_performance_benchmark.ps1.
	void ConfigureCommandLineBenchmark();
	void TickCommandLineBenchmark(float DeltaSeconds);

	enum class EBenchmarkCaptureState : uint8
	{
		Disabled,
		Warmup,
		Capturing,
		Finishing
	};

	EBenchmarkCaptureState BenchmarkCaptureState = EBenchmarkCaptureState::Disabled;
	float BenchmarkElapsedSeconds = 0.0f;
	float BenchmarkWarmupSeconds = 0.0f;
	float BenchmarkCaptureSeconds = 0.0f;
	FString BenchmarkCsvLabel;

	void BindDebugBufferHotkeys();
	void SelectRadianceDebugBuffer();
	void SelectTemperatureDebugBuffer();
	void SelectEmissivityDebugBuffer();
	void SelectDepthDebugBuffer();
	void SelectNormalsDebugBuffer();
	void SelectMaterialIdDebugBuffer();
	void EnsureRenderTarget();
	void CaptureAuxiliaryBuffers();
	void CaptureSceneToTarget(UTextureRenderTarget2D* Target, ESceneCaptureSource Source);
	void CaptureThermalMaterialToTarget(UTextureRenderTarget2D* Target, UMaterialInterface* BufferMaterial, ESceneCaptureSource Source = SCS_SceneColorHDRNoAlpha);
	void SyncToPlayerCamera();
	void UpdatePlayerCameraView();
	UTextureRenderTarget2D* GetDebugRenderTarget() const;
	void GetDebugDisplayRange(float& OutMin, float& OutMax) const;
	void ClearPlayerCameraView();
	UCameraComponent* FindPlayerCameraComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicPlayerViewMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> BoundPlayerCameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UIRInternalSceneCaptureComponent> AuxiliaryCaptureComponent;

	bool bDebugBufferHotkeysBound = false;
};
