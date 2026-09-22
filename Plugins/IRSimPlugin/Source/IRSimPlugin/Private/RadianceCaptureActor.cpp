// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadianceCaptureActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "IRThermalSurfaceComponent.h"
#include "IRInternalSceneCaptureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "RenderingThread.h"

// Capturador de la salida radiometrica del sensor virtual
// La vista de debug se mantiene separada de los datos fisicos capturados

ARadianceCaptureActor::ARadianceCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CaptureComponent = CreateDefaultSubobject<UIRInternalSceneCaptureComponent>(TEXT("RadianceCapture"));
	SetRootComponent(CaptureComponent);

	PlayerViewPostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PlayerViewPostProcess"));
	PlayerViewPostProcessComponent->SetupAttachment(CaptureComponent);
	PlayerViewPostProcessComponent->bUnbound = true;
	PlayerViewPostProcessComponent->bEnabled = false;
	PlayerViewPostProcessComponent->BlendWeight = 1.0f;

	AuxiliaryCaptureComponent = CreateDefaultSubobject<UIRInternalSceneCaptureComponent>(TEXT("AuxiliaryCapture"));
	AuxiliaryCaptureComponent->SetupAttachment(CaptureComponent);
	AuxiliaryCaptureComponent->bCaptureEveryFrame = false;
	AuxiliaryCaptureComponent->bCaptureOnMovement = false;
	AuxiliaryCaptureComponent->bAlwaysPersistRenderingState = false;

	CaptureComponent->CaptureSource = SCS_SceneColorHDRNoAlpha;
	CaptureComponent->PostProcessBlendWeight = 0.0f;
	CaptureComponent->bAlwaysPersistRenderingState = false;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
}

void ARadianceCaptureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshCapturePipeline();
}

void ARadianceCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	// Los mapas creados antes de ampliar la banda LWIR conservan el antiguo
	// maximo de visualizacion (1.0). No afecta a la radiancia fisica, pero la
	// saturaba por completo al pasar a 8--14 um.
	if (FMath::IsNearlyEqual(DisplayRadianceMax, 1.0f))
	{
		DisplayRadianceMax = 130.0f;
	}

	// Permite medir la pasada de radiancia de forma aislada sin modificar el
	// mapa ni la configuracion persistente de sus buffers auxiliares.
	if (FParse::Param(FCommandLine::Get(), TEXT("IRRadianceOnly")))
	{
		bCaptureAuxiliaryBuffers = false;
		UE_LOG(LogTemp, Display, TEXT("IR benchmark: solo se captura el buffer de radiancia."));
	}

	RefreshCapturePipeline();
	ConfigureCommandLineBenchmark();
	BindDebugBufferHotkeys();
}

void ARadianceCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPlayerCameraView();
	if (bDebugBufferHotkeysBound)
	{
		DisableInput(UGameplayStatics::GetPlayerController(this, 0));
		bDebugBufferHotkeysBound = false;
	}

	Super::EndPlay(EndPlayReason);
}

void ARadianceCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickCommandLineBenchmark(DeltaSeconds);

	if (!bEnableRadianceCapture)
	{
		return;
	}

	if (bFollowPlayerCamera || FollowCameraActor)
	{
		SyncToPlayerCamera();
	}

	CaptureRadianceFrame();
}

void ARadianceCaptureActor::CaptureRadianceFrame()
{
	// Actualizamos la captura de forma explicita para que el resultado corresponda
	// al mismo estado de la escena que se esta validando
	RefreshCapturePipeline();

	if (CaptureComponent)
	{
		const bool bHadPlayerViewPostProcess =
			PlayerViewPostProcessComponent && PlayerViewPostProcessComponent->bEnabled;
		if (PlayerViewPostProcessComponent)
		{
			PlayerViewPostProcessComponent->bEnabled = false;
		}

		CaptureComponent->TextureTarget = RadianceRenderTarget;
		CaptureComponent->MarkRenderStateDirty();
		if (RadianceRenderTarget)
		{
			RadianceRenderTarget->UpdateResourceImmediate(false);
		}
		CaptureComponent->CaptureScene();
		FlushRenderingCommands();

		if (bCaptureAuxiliaryBuffers)
		{
			CaptureAuxiliaryBuffers();
		}


		if (RadianceRenderTarget)
		{
			OnRadianceFrameCaptured.Broadcast(RadianceRenderTarget);
		}

		if (bHadPlayerViewPostProcess)
		{
			UpdatePlayerCameraView();
		}
	}
}

void ARadianceCaptureActor::ConfigureCommandLineBenchmark()
{
	FString Mode;
	if (!FParse::Value(FCommandLine::Get(), TEXT("IRBenchmarkMode="), Mode))
	{
		return;
	}

	Mode = Mode.ToLower();
	if (Mode == TEXT("base"))
	{
		bEnableRadianceCapture = false;
		bCaptureAuxiliaryBuffers = false;
		bShowRenderTargetOnPlayerCamera = false;
	}
	else if (Mode == TEXT("radiance"))
	{
		bEnableRadianceCapture = true;
		bCaptureAuxiliaryBuffers = false;
		bShowRenderTargetOnPlayerCamera = false;
	}
	else if (Mode == TEXT("visible"))
	{
		bEnableRadianceCapture = true;
		bCaptureAuxiliaryBuffers = false;
		bShowRenderTargetOnPlayerCamera = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("IR benchmark mode '%s' is invalid. Use base, radiance or visible."), *Mode);
		return;
	}

	BenchmarkWarmupSeconds = 10.0f;
	BenchmarkCaptureSeconds = 30.0f;
	FParse::Value(FCommandLine::Get(), TEXT("IRBenchmarkWarmup="), BenchmarkWarmupSeconds);
	FParse::Value(FCommandLine::Get(), TEXT("IRBenchmarkSeconds="), BenchmarkCaptureSeconds);
	if (BenchmarkWarmupSeconds < 0.0f || BenchmarkCaptureSeconds <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("IR benchmark durations must be non-negative warmup and positive capture seconds."));
		return;
	}

	BenchmarkCsvLabel = FString::Printf(TEXT("ir_%s"), *Mode);
	BenchmarkCaptureState = EBenchmarkCaptureState::Warmup;
	BenchmarkElapsedSeconds = 0.0f;
	UE_LOG(LogTemp, Display, TEXT("IR benchmark configured: mode=%s warmup=%.1fs capture=%.1fs."),
		*Mode, BenchmarkWarmupSeconds, BenchmarkCaptureSeconds);
}

void ARadianceCaptureActor::TickCommandLineBenchmark(float DeltaSeconds)
{
	if (BenchmarkCaptureState == EBenchmarkCaptureState::Disabled || !GetWorld() || !GEngine)
	{
		return;
	}

	BenchmarkElapsedSeconds += DeltaSeconds;
	if (BenchmarkCaptureState == EBenchmarkCaptureState::Warmup && BenchmarkElapsedSeconds >= BenchmarkWarmupSeconds)
	{
		// CSV is the Unreal-owned raw evidence.  The launcher script copies the
		// newly created file to a deterministic result directory.
		GEngine->Exec(GetWorld(), TEXT("csvprofile start"));
		BenchmarkCaptureState = EBenchmarkCaptureState::Capturing;
		BenchmarkElapsedSeconds = 0.0f;
		UE_LOG(LogTemp, Display, TEXT("IR benchmark CSV capture started (%s)."), *BenchmarkCsvLabel);
	}
	else if (BenchmarkCaptureState == EBenchmarkCaptureState::Capturing && BenchmarkElapsedSeconds >= BenchmarkCaptureSeconds)
	{
		GEngine->Exec(GetWorld(), TEXT("csvprofile stop"));
		BenchmarkCaptureState = EBenchmarkCaptureState::Finishing;
		BenchmarkElapsedSeconds = 0.0f;
		UE_LOG(LogTemp, Display, TEXT("IR benchmark CSV capture stopped (%s)."), *BenchmarkCsvLabel);
	}
	else if (BenchmarkCaptureState == EBenchmarkCaptureState::Finishing && BenchmarkElapsedSeconds >= 3.0f)
	{
		BenchmarkCaptureState = EBenchmarkCaptureState::Disabled;
		GEngine->Exec(GetWorld(), TEXT("quit"));
	}
}

void ARadianceCaptureActor::SetDebugBuffer(EIRDebugBuffer InDebugBuffer)
{
	DebugBuffer = InDebugBuffer;
	// Bind the selected target and its appropriate display range immediately;
	// the source targets remain unmodified raw measurements.
	UpdatePlayerCameraView();
}

EIRDebugBuffer ARadianceCaptureActor::GetDebugBuffer() const
{
	return DebugBuffer;
}

void ARadianceCaptureActor::SetDisplayRadianceRange(float InMin, float InMax)
{
	DisplayRadianceMin = FMath::Max(InMin, 0.0f);
	DisplayRadianceMax = FMath::Max(InMax, DisplayRadianceMin + KINDA_SMALL_NUMBER);
	UpdatePlayerCameraView();
}

void ARadianceCaptureActor::BindDebugBufferHotkeys()
{
	if (!bEnableDebugBufferHotkeys || bDebugBufferHotkeysBound || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("IR debug buffer hotkeys were not bound: player controller 0 is unavailable."));
		return;
	}

	EnableInput(PlayerController);
	if (!InputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("IR debug buffer hotkeys were not bound: input component is unavailable."));
		return;
	}

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ARadianceCaptureActor::SelectRadianceDebugBuffer);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ARadianceCaptureActor::SelectTemperatureDebugBuffer);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ARadianceCaptureActor::SelectEmissivityDebugBuffer);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ARadianceCaptureActor::SelectDepthDebugBuffer);
	InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ARadianceCaptureActor::SelectNormalsDebugBuffer);
	InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ARadianceCaptureActor::SelectMaterialIdDebugBuffer);
	bDebugBufferHotkeysBound = true;
}

void ARadianceCaptureActor::SelectRadianceDebugBuffer()
{
	SetDebugBuffer(EIRDebugBuffer::Radiance);
}

void ARadianceCaptureActor::SelectTemperatureDebugBuffer()
{
	SetDebugBuffer(EIRDebugBuffer::Temperature);
}

void ARadianceCaptureActor::SelectEmissivityDebugBuffer()
{
	SetDebugBuffer(EIRDebugBuffer::Emissivity);
}

void ARadianceCaptureActor::SelectDepthDebugBuffer()
{
	SetDebugBuffer(EIRDebugBuffer::Depth);
}

void ARadianceCaptureActor::SelectNormalsDebugBuffer()
{
	SetDebugBuffer(EIRDebugBuffer::Normals);
}

void ARadianceCaptureActor::SelectMaterialIdDebugBuffer()
{
	SetDebugBuffer(EIRDebugBuffer::MaterialId);
}

void ARadianceCaptureActor::SetAuxiliaryBufferMaterials(
	UMaterialInterface* InTemperatureMaterial,
	UMaterialInterface* InEmissivityMaterial,
	UMaterialInterface* InMaterialIdMaterial)
{
	TemperatureBufferMaterial = InTemperatureMaterial;
	EmissivityBufferMaterial = InEmissivityMaterial;
	MaterialIdBufferMaterial = InMaterialIdMaterial;
}

void ARadianceCaptureActor::CaptureSceneToTarget(
	UTextureRenderTarget2D* Target,
	ESceneCaptureSource Source)
{
	if (!AuxiliaryCaptureComponent || !Target)
	{
		return;
	}

	AuxiliaryCaptureComponent->TextureTarget = Target;
	AuxiliaryCaptureComponent->CaptureSource = Source;
	AuxiliaryCaptureComponent->PostProcessBlendWeight = 0.0f;
	AuxiliaryCaptureComponent->bAlwaysPersistRenderingState = true;
	AuxiliaryCaptureComponent->ShowFlags.SetPostProcessing(false);
	AuxiliaryCaptureComponent->ShowFlags.SetTonemapper(false);
	AuxiliaryCaptureComponent->ShowFlags.SetEyeAdaptation(false);
	AuxiliaryCaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
#if WITH_DEV_AUTOMATION_TESTS
	if (GIsAutomationTesting)
	{
		UE_LOG(LogTemp, Display, TEXT("IR target binding capture=%s source=%d destination=%s same_target=%d"),
			*AuxiliaryCaptureComponent->GetPathName(), int32(Source), *Target->GetPathName(), AuxiliaryCaptureComponent->TextureTarget == Target);
	}
#endif
	AuxiliaryCaptureComponent->CaptureScene();
	FlushRenderingCommands();
}

void ARadianceCaptureActor::CaptureThermalMaterialToTarget(
	UTextureRenderTarget2D* Target,
	UMaterialInterface* BufferMaterial,
	ESceneCaptureSource Source)
{
	if (!Target || !BufferMaterial || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("IR auxiliary buffer capture skipped: missing target, material or world."));
		return;
	}

	struct FMaterialRestore
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TArray<TObjectPtr<UMaterialInterface>> Materials;
	};

#if WITH_DEV_AUTOMATION_TESTS
	if (GIsAutomationTesting)
	{
		UE_LOG(LogTemp, Display, TEXT("IR target material destination=%s applied_material=%s source=%d"),
			*Target->GetPathName(), *BufferMaterial->GetPathName(), int32(Source));
	}
#endif
	TArray<FMaterialRestore> Restores;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		UIRThermalSurfaceComponent* ThermalSurface = It->FindComponentByClass<UIRThermalSurfaceComponent>();
		UStaticMeshComponent* Mesh = ThermalSurface ? ThermalSurface->GetTargetMesh() : nullptr;
		if (!Mesh)
		{
			continue;
		}

		FMaterialRestore& Restore = Restores.AddDefaulted_GetRef();
		Restore.Mesh = Mesh;
		for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
		{
			Restore.Materials.Add(Mesh->GetMaterial(MaterialIndex));
			Mesh->SetMaterial(MaterialIndex, BufferMaterial);
		}
	}

	// SetMaterial only queues the primitive render-state update. Wait until the
	// replacement material is visible to the render thread before capturing.
	FlushRenderingCommands();
	CaptureSceneToTarget(Target, Source);

	for (const FMaterialRestore& Restore : Restores)
	{
		if (!Restore.Mesh)
		{
			continue;
		}
		for (int32 MaterialIndex = 0; MaterialIndex < Restore.Materials.Num(); ++MaterialIndex)
		{
			Restore.Mesh->SetMaterial(MaterialIndex, Restore.Materials[MaterialIndex]);
		}
		Restore.Mesh->MarkRenderStateDirty();
	}
	FlushRenderingCommands();
}

void ARadianceCaptureActor::CaptureAuxiliaryBuffers()
{
	// Temperature, emissivity and material ID are material-output passes. The
	// corresponding materials are deliberately user-authored shells.
	if (!TemperatureBufferMaterial || !EmissivityBufferMaterial || !MaterialIdBufferMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("IR auxiliary buffer material is not assigned on RadianceCaptureActor."));
	}
	CaptureThermalMaterialToTarget(TemperatureRenderTarget, TemperatureBufferMaterial);
	CaptureThermalMaterialToTarget(EmissivityRenderTarget, EmissivityBufferMaterial);
	CaptureThermalMaterialToTarget(MaterialIdRenderTarget, MaterialIdBufferMaterial);

	// Geometry buffers use Unreal's native scene capture sources.
	CaptureSceneToTarget(DepthRenderTarget, SCS_SceneDepth);
	// Unlit thermal materials do not populate GBuffer normals (UE5.6
	// BasePassPixelShader.usf clears MRT[1]). A flat lit material is required
	// only during this geometry pass; restore all original slots afterwards.
	CaptureThermalMaterialToTarget(NormalRenderTarget,
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), SCS_Normal);
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetRadianceRenderTarget() const
{
	return RadianceRenderTarget;
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetTemperatureRenderTarget() const
{
	return TemperatureRenderTarget;
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetEmissivityRenderTarget() const
{
	return EmissivityRenderTarget;
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetDepthRenderTarget() const
{
	return DepthRenderTarget;
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetNormalRenderTarget() const
{
	return NormalRenderTarget;
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetMaterialIdRenderTarget() const
{
	return MaterialIdRenderTarget;
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetDebugRenderTarget() const
{
	switch (DebugBuffer)
	{
	case EIRDebugBuffer::Temperature:
		return TemperatureRenderTarget;
	case EIRDebugBuffer::Emissivity:
		return EmissivityRenderTarget;
	case EIRDebugBuffer::Depth:
		return DepthRenderTarget;
	case EIRDebugBuffer::Normals:
		return NormalRenderTarget;
	case EIRDebugBuffer::MaterialId:
		return MaterialIdRenderTarget;
	case EIRDebugBuffer::Radiance:
	default:
		return RadianceRenderTarget;
	}
}

void ARadianceCaptureActor::GetDebugDisplayRange(float& OutMin, float& OutMax) const
{
	OutMin = DisplayRadianceMin;
	OutMax = DisplayRadianceMax;

	switch (DebugBuffer)
	{
	case EIRDebugBuffer::Emissivity:
	case EIRDebugBuffer::Normals:
		OutMin = 0.0f;
		OutMax = 1.0f;
		break;
	case EIRDebugBuffer::Depth:
		// SCS_SceneDepth is expressed in Unreal units (centimetres).
		OutMin = 0.0f;
		OutMax = FMath::Max(DebugDepthMaxCentimeters, 1.0f);
		break;
	case EIRDebugBuffer::MaterialId:
		OutMin = 0.0f;
		OutMax = FMath::Max(DebugMaterialIdMax, 1.0f);
		break;
	case EIRDebugBuffer::Temperature:
		// Temperature is stored in kelvin; its range remains user-configurable.
		break;
	case EIRDebugBuffer::Radiance:
	default:
		break;
	}
}

FVector ARadianceCaptureActor::GetSensorWorldLocation() const
{
	return CaptureComponent ? CaptureComponent->GetComponentLocation() : GetActorLocation();
}

bool ARadianceCaptureActor::ProjectWorldLocationToRenderTarget(
	const FVector& WorldLocation,
	FIntPoint& OutPixel) const
{
	// Esta proyeccion permite elegir un pixel conocido para las pruebas CPU GPU
	if (!CaptureComponent || !RadianceRenderTarget)
	{
		return false;
	}

	const int32 Width = RadianceRenderTarget->SizeX;
	const int32 Height = RadianceRenderTarget->SizeY;
	if (Width <= 0 || Height <= 0)
	{
		return false;
	}

	const FVector LocalPosition = CaptureComponent->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
	float NdcX = 0.0f;
	float NdcY = 0.0f;

	if (CaptureComponent->ProjectionType == ECameraProjectionMode::Perspective)
	{
		if (LocalPosition.X <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const float TanHalfHorizontalFov = FMath::Tan(FMath::DegreesToRadians(CaptureComponent->FOVAngle * 0.5f));
		NdcX = LocalPosition.Y / (LocalPosition.X * TanHalfHorizontalFov);
		NdcY = LocalPosition.Z / (LocalPosition.X * TanHalfHorizontalFov / AspectRatio);
	}
	else
	{
		const float HalfWidth = CaptureComponent->OrthoWidth * 0.5f;
		if (HalfWidth <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		NdcX = LocalPosition.Y / HalfWidth;
		NdcY = LocalPosition.Z / (HalfWidth / AspectRatio);
	}

	if (FMath::Abs(NdcX) > 1.0f || FMath::Abs(NdcY) > 1.0f)
	{
		return false;
	}

	OutPixel.X = FMath::RoundToInt((NdcX + 1.0f) * 0.5f * static_cast<float>(Width - 1));
	OutPixel.Y = FMath::RoundToInt((1.0f - NdcY) * 0.5f * static_cast<float>(Height - 1));
	return true;
}

void ARadianceCaptureActor::RefreshCapturePipeline()
{
	// Reconfiguramos recursos y materiales despues de cambiar parametros del actor
	EnsureRenderTarget();
	UpdatePlayerCameraView();

	if (CaptureComponent)
	{
		// La captura fisica no debe contener transformaciones de pantalla ni ajustes dependientes de exposicion
		CaptureComponent->PostProcessSettings.WeightedBlendables.Array.Reset();
		CaptureComponent->PostProcessSettings.bOverride_AutoExposureMethod = true;
		CaptureComponent->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		CaptureComponent->PostProcessSettings.bOverride_AutoExposureBias = true;
		CaptureComponent->PostProcessSettings.AutoExposureBias = 0.0f;
		CaptureComponent->ShowFlags.SetPostProcessing(false);
		CaptureComponent->ShowFlags.SetTonemapper(false);
		CaptureComponent->ShowFlags.SetEyeAdaptation(false);
	}
}

void ARadianceCaptureActor::EnsureRenderTarget()
{
	// El formato flotante conserva la magnitud de radiancia sin convertirla a color
	if (!CaptureComponent)
	{
		return;
	}

	// Keep the auxiliary capture configuration self-contained. The assets are
	// existing user-authored buffer materials; this only resolves missing
	// references on the actor and never edits their graphs.
	if (!TemperatureBufferMaterial)
	{
		TemperatureBufferMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/IRSimPlugin/Materials/Buffers/M_IR_Output_Temperature.M_IR_Output_Temperature"));
	}
	if (!EmissivityBufferMaterial)
	{
		EmissivityBufferMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/IRSimPlugin/Materials/Buffers/M_IR_Output_Emissivity.M_IR_Output_Emissivity"));
	}
	if (!MaterialIdBufferMaterial)
	{
		MaterialIdBufferMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/IRSimPlugin/Materials/Buffers/M_IR_Output_MaterialId.M_IR_Output_MaterialId"));
	}
	// SceneCapture2D escribe un color de escena float4 y RGBA16F conserva la captura
	// fisica mientras el canal R mantiene la radiancia del contrato de salida
	const ETextureRenderTargetFormat DesiredRenderTargetFormat = RTF_RGBA16f;
	const EPixelFormat DesiredPixelFormat = PF_FloatRGBA;
	const bool bNeedsRecreate = !RadianceRenderTarget
		|| RadianceRenderTarget->SizeX != TargetWidth
		|| RadianceRenderTarget->SizeY != TargetHeight
		|| RadianceRenderTarget->RenderTargetFormat != DesiredRenderTargetFormat;

	if (bNeedsRecreate)
	{
		RadianceRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("RadianceRenderTarget"));
		RadianceRenderTarget->RenderTargetFormat = DesiredRenderTargetFormat;
		RadianceRenderTarget->ClearColor = FLinearColor::Black;
		RadianceRenderTarget->bAutoGenerateMips = false;
		RadianceRenderTarget->SRGB = false;
		RadianceRenderTarget->TargetGamma = 1.0f;
		RadianceRenderTarget->InitCustomFormat(TargetWidth, TargetHeight, DesiredPixelFormat, true);
		RadianceRenderTarget->UpdateResourceImmediate(true);
	}

	// Auxiliary buffers use linear floating-point formats and are kept separate
	// from the physical radiance buffer. Their contents are written by the
	// corresponding buffer capture/material pass, never by the debug palette.
	auto EnsureAuxiliaryTarget = [this](
		TObjectPtr<UTextureRenderTarget2D>& Target,
		const TCHAR* Name,
		EPixelFormat PixelFormat,
		ETextureRenderTargetFormat RenderTargetFormat)
	{
		const bool bNeedsAuxiliaryRecreate = !Target
			|| Target->SizeX != TargetWidth
			|| Target->SizeY != TargetHeight
			|| Target->RenderTargetFormat != RenderTargetFormat;

		if (bNeedsAuxiliaryRecreate)
		{
			Target = NewObject<UTextureRenderTarget2D>(this, Name);
			Target->RenderTargetFormat = RenderTargetFormat;
			Target->ClearColor = FLinearColor::Black;
			Target->bAutoGenerateMips = false;
			Target->SRGB = false;
			Target->TargetGamma = 1.0f;
			Target->InitCustomFormat(TargetWidth, TargetHeight, PixelFormat, true);
			Target->UpdateResourceImmediate(true);
		}
	};

	EnsureAuxiliaryTarget(TemperatureRenderTarget, TEXT("TemperatureRenderTarget"), PF_FloatRGBA, RTF_RGBA16f);
	EnsureAuxiliaryTarget(EmissivityRenderTarget, TEXT("EmissivityRenderTarget"), PF_FloatRGBA, RTF_RGBA16f);
	EnsureAuxiliaryTarget(DepthRenderTarget, TEXT("DepthRenderTarget"), PF_R32_FLOAT, RTF_R32f);
	EnsureAuxiliaryTarget(NormalRenderTarget, TEXT("NormalRenderTarget"), PF_FloatRGBA, RTF_RGBA16f);
	EnsureAuxiliaryTarget(MaterialIdRenderTarget, TEXT("MaterialIdRenderTarget"), PF_R32_FLOAT, RTF_R32f);

	CaptureComponent->TextureTarget = RadianceRenderTarget;
	CaptureComponent->CaptureSource = SCS_SceneColorHDRNoAlpha;
	CaptureComponent->PostProcessBlendWeight = 0.0f;
	CaptureComponent->ShowFlags.SetPostProcessing(false);
		CaptureComponent->ShowFlags.SetTonemapper(false);
		CaptureComponent->ShowFlags.SetEyeAdaptation(false);
	CaptureComponent->bAlwaysPersistRenderingState = false;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
}

void ARadianceCaptureActor::SyncToPlayerCamera()
{
	if (FollowCameraActor)
	{
		SetActorLocationAndRotation(
			FollowCameraActor->GetActorLocation(),
			FollowCameraActor->GetActorRotation());
		return;
	}

	// PlayerCameraManager resuelve la vista activa. Esto incluye una
	// CineCameraActor tomada por Level Sequencer, mientras que la camara del
	// Pawn solo representa la vista por defecto fuera de una secuencia.
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		SetActorLocationAndRotation(
			PlayerController->PlayerCameraManager->GetCameraLocation(),
			PlayerController->PlayerCameraManager->GetCameraRotation());
		return;
	}

	UCameraComponent* PlayerCameraComponent = FindPlayerCameraComponent();
	if (PlayerCameraComponent)
	{
		SetActorLocationAndRotation(
			PlayerCameraComponent->GetComponentLocation(),
			PlayerCameraComponent->GetComponentRotation());
	}
}

void ARadianceCaptureActor::UpdatePlayerCameraView()
{
	const bool bShouldShowOnPlayerCamera =
		bShowRenderTargetOnPlayerCamera && PlayerViewPostProcessMaterial && RadianceRenderTarget;

	UCameraComponent* PlayerCameraComponent = FindPlayerCameraComponent();
	if (!bShouldShowOnPlayerCamera)
	{
		ClearPlayerCameraView();
		return;
	}

	if (!DynamicPlayerViewMaterial || DynamicPlayerViewMaterial->Parent != PlayerViewPostProcessMaterial)
	{
		if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
		{
			BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		}

		DynamicPlayerViewMaterial = UMaterialInstanceDynamic::Create(PlayerViewPostProcessMaterial, this);
		BoundPlayerCameraComponent = nullptr;
	}

	if (PlayerCameraComponent && BoundPlayerCameraComponent != PlayerCameraComponent)
	{
		if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
		{
			BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		}

		if (DynamicPlayerViewMaterial)
		{
			PlayerCameraComponent->PostProcessSettings.AddBlendable(DynamicPlayerViewMaterial, 1.0f);
			BoundPlayerCameraComponent = PlayerCameraComponent;
		}
	}
	else if (!PlayerCameraComponent && BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
	{
		BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		BoundPlayerCameraComponent = nullptr;
	}

	if (DynamicPlayerViewMaterial)
	{
		float DebugMin = 0.0f;
		float DebugMax = 1.0f;
		GetDebugDisplayRange(DebugMin, DebugMax);
		DynamicPlayerViewMaterial->SetTextureParameterValue(PlayerViewTextureParameterName, GetDebugRenderTarget());
		DynamicPlayerViewMaterial->SetScalarParameterValue(TEXT("DisplayRadianceMin"), DebugMin);
		DynamicPlayerViewMaterial->SetScalarParameterValue(TEXT("DisplayRadianceMax"), DebugMax);

		if (PlayerViewPostProcessComponent)
		{
			PlayerViewPostProcessComponent->Settings.WeightedBlendables.Array.Reset();
			PlayerViewPostProcessComponent->bEnabled = false;

			if (!PlayerCameraComponent && GetWorld() && GetWorld()->IsGameWorld())
			{
				PlayerViewPostProcessComponent->bUnbound = true;
				PlayerViewPostProcessComponent->BlendWeight = 1.0f;
				PlayerViewPostProcessComponent->Settings.AddBlendable(DynamicPlayerViewMaterial, 1.0f);
				PlayerViewPostProcessComponent->bEnabled = true;
			}
		}
	}
}

void ARadianceCaptureActor::ClearPlayerCameraView()
{
	if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
	{
		BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
	}

	if (PlayerViewPostProcessComponent)
	{
		PlayerViewPostProcessComponent->Settings.WeightedBlendables.Array.Reset();
		PlayerViewPostProcessComponent->bEnabled = false;
	}

	BoundPlayerCameraComponent = nullptr;
	DynamicPlayerViewMaterial = nullptr;
}

UCameraComponent* ARadianceCaptureActor::FindPlayerCameraComponent() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return nullptr;
	}

	APawn* PlayerPawn = PlayerController->GetPawn();
	return PlayerPawn ? PlayerPawn->FindComponentByClass<UCameraComponent>() : nullptr;
}
