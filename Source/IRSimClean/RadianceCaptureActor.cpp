// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadianceCaptureActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ARadianceCaptureActor::ARadianceCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

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

void ARadianceCaptureActor::RefreshCapturePipeline()
{
	EnsureRenderTarget();
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
