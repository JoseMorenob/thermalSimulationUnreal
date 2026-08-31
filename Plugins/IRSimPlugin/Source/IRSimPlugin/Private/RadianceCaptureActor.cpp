// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadianceCaptureActor.h"

#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RenderingThread.h"

// Capturador de la salida radiometrica del sensor virtual
// La vista de debug se mantiene separada de los datos fisicos capturados

ARadianceCaptureActor::ARadianceCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("RadianceCapture"));
	SetRootComponent(CaptureComponent);

	PlayerViewPostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PlayerViewPostProcess"));
	PlayerViewPostProcessComponent->SetupAttachment(CaptureComponent);
	PlayerViewPostProcessComponent->bUnbound = true;
	PlayerViewPostProcessComponent->bEnabled = false;
	PlayerViewPostProcessComponent->BlendWeight = 1.0f;

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
	RefreshCapturePipeline();
}

void ARadianceCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPlayerCameraView();

	Super::EndPlay(EndPlayReason);
}

void ARadianceCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFollowPlayerCamera)
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

		if (bHadPlayerViewPostProcess)
		{
			UpdatePlayerCameraView();
		}
	}
}

UTextureRenderTarget2D* ARadianceCaptureActor::GetRadianceRenderTarget() const
{
	return RadianceRenderTarget;
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
	}
}

void ARadianceCaptureActor::EnsureRenderTarget()
{
	// El formato flotante conserva la magnitud de radiancia sin convertirla a color
	if (!CaptureComponent)
	{
		return;
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
		RadianceRenderTarget->InitCustomFormat(TargetWidth, TargetHeight, DesiredPixelFormat, true);
		RadianceRenderTarget->UpdateResourceImmediate(true);
	}

	CaptureComponent->TextureTarget = RadianceRenderTarget;
	CaptureComponent->CaptureSource = SCS_SceneColorHDRNoAlpha;
	CaptureComponent->PostProcessBlendWeight = 0.0f;
	CaptureComponent->bAlwaysPersistRenderingState = false;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
}

void ARadianceCaptureActor::SyncToPlayerCamera()
{
	// La camara del jugador solo se usa para facilitar la inspeccion visual
	UCameraComponent* PlayerCameraComponent = FindPlayerCameraComponent();
	if (PlayerCameraComponent)
	{
		SetActorLocationAndRotation(
			PlayerCameraComponent->GetComponentLocation(),
			PlayerCameraComponent->GetComponentRotation());
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		SetActorLocationAndRotation(
			PlayerController->PlayerCameraManager->GetCameraLocation(),
			PlayerController->PlayerCameraManager->GetCameraRotation());
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
		DynamicPlayerViewMaterial->SetTextureParameterValue(PlayerViewTextureParameterName, RadianceRenderTarget);
		DynamicPlayerViewMaterial->SetScalarParameterValue(TEXT("DisplayRadianceMin"), DisplayRadianceMin);
		DynamicPlayerViewMaterial->SetScalarParameterValue(TEXT("DisplayRadianceMax"), DisplayRadianceMax);
		DynamicPlayerViewMaterial->SetScalarParameterValue(TEXT("InvertDebugDisplay"), bInvertDebugDisplay ? 1.0f : 0.0f);

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
