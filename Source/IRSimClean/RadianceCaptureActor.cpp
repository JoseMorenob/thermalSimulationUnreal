// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadianceCaptureActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ARadianceCaptureActor::ARadianceCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("RadianceCapture"));
	SetRootComponent(CaptureComponent);

	CaptureComponent->CaptureSource = SCS_FinalColorHDR;
	CaptureComponent->bCaptureEveryFrame = bCaptureEveryFrame;
	CaptureComponent->bCaptureOnMovement = bCaptureOnMovement;
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
	if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
	{
		BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
	}

	BoundPlayerCameraComponent = nullptr;
	DynamicPlayerViewMaterial = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ARadianceCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFollowPlayerCamera)
	{
		SyncToPlayerCamera();
	}

	if (bShowRenderTargetOnPlayerCamera || DynamicPlayerViewMaterial)
	{
		UpdatePlayerCameraView();
	}
}

void ARadianceCaptureActor::CaptureRadianceFrame()
{
	RefreshCapturePipeline();

	if (CaptureComponent)
	{
		CaptureComponent->TextureTarget = RadianceRenderTarget;
		CaptureComponent->MarkRenderStateDirty();
		if (RadianceRenderTarget)
		{
			RadianceRenderTarget->UpdateResourceImmediate(false);
		}
		CaptureComponent->CaptureScene();
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

void ARadianceCaptureActor::RefreshCapturePipeline()
{
	EnsureRenderTarget();
	UpdatePlayerCameraView();

	if (!CaptureComponent)
	{
		return;
	}

	if (!PostProcessThermalMaterial)
	{
		CaptureComponent->PostProcessSettings.WeightedBlendables.Array.Reset();
		DynamicPostProcessMaterial = nullptr;
		return;
	}

	if (!DynamicPostProcessMaterial || DynamicPostProcessMaterial->Parent != PostProcessThermalMaterial)
	{
		DynamicPostProcessMaterial = UMaterialInstanceDynamic::Create(PostProcessThermalMaterial, this);
	}

	if (DynamicPostProcessMaterial)
	{
		DynamicPostProcessMaterial->SetScalarParameterValue(TEXT("RadianceNormalizationMax"), RadianceNormalizationMax);
		DynamicPostProcessMaterial->SetScalarParameterValue(TEXT("BandMinMicrons"), BandMinMicrons);
		DynamicPostProcessMaterial->SetScalarParameterValue(TEXT("BandMaxMicrons"), BandMaxMicrons);

		CaptureComponent->PostProcessSettings.WeightedBlendables.Array.Reset();
		CaptureComponent->PostProcessSettings.AddBlendable(DynamicPostProcessMaterial, 1.0f);
	}
}

void ARadianceCaptureActor::EnsureRenderTarget()
{
	if (!CaptureComponent)
	{
		return;
	}

	const ETextureRenderTargetFormat DesiredRenderTargetFormat = bUseSingleChannelRenderTarget ? RTF_R32f : RTF_RGBA16f;
	const EPixelFormat DesiredPixelFormat = bUseSingleChannelRenderTarget ? PF_R32_FLOAT : PF_FloatRGBA;
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
	CaptureComponent->bCaptureEveryFrame = bCaptureEveryFrame;
	CaptureComponent->bCaptureOnMovement = bCaptureOnMovement;
}

void ARadianceCaptureActor::SyncToPlayerCamera()
{
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
	UCameraComponent* PlayerCameraComponent = FindPlayerCameraComponent();
	if (!PlayerCameraComponent)
	{
		if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
		{
			BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		}
		BoundPlayerCameraComponent = nullptr;
		DynamicPlayerViewMaterial = nullptr;
		return;
	}

	const bool bShouldShowOnPlayerCamera =
		bShowRenderTargetOnPlayerCamera && PlayerViewPostProcessMaterial && RadianceRenderTarget;

	if (!bShouldShowOnPlayerCamera)
	{
		if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
		{
			BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		}
		BoundPlayerCameraComponent = nullptr;
		DynamicPlayerViewMaterial = nullptr;
		return;
	}

	if (!DynamicPlayerViewMaterial || DynamicPlayerViewMaterial->Parent != PlayerViewPostProcessMaterial)
	{
		if (BoundPlayerCameraComponent && DynamicPlayerViewMaterial)
		{
			BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		}

		DynamicPlayerViewMaterial = UMaterialInstanceDynamic::Create(PlayerViewPostProcessMaterial, this);
		PlayerCameraComponent->PostProcessSettings.AddBlendable(DynamicPlayerViewMaterial, 1.0f);
		BoundPlayerCameraComponent = PlayerCameraComponent;
	}
	else if (BoundPlayerCameraComponent != PlayerCameraComponent)
	{
		if (BoundPlayerCameraComponent)
		{
			BoundPlayerCameraComponent->PostProcessSettings.RemoveBlendable(DynamicPlayerViewMaterial);
		}
		PlayerCameraComponent->PostProcessSettings.AddBlendable(DynamicPlayerViewMaterial, 1.0f);
		BoundPlayerCameraComponent = PlayerCameraComponent;
	}

	if (DynamicPlayerViewMaterial)
	{
		DynamicPlayerViewMaterial->SetTextureParameterValue(PlayerViewTextureParameterName, RadianceRenderTarget);
	}
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
