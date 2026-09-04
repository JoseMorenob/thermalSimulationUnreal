// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRRadianceValidationActor.h"

#include "Engine/TextureRenderTarget2D.h"
#include "IRThermalSurfaceComponent.h"
#include "Materials/MaterialInterface.h"
#include "RadianceCaptureActor.h"
#include "TextureResource.h"

// Validacion CPU frente al pixel fisico del Render Target.

// Herramienta de comprobacion para comparar el valor CPU con el pixel generado
// por el material fisico en el render target

AIRRadianceValidationActor::AIRRadianceValidationActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AIRRadianceValidationActor::ConfigureSurfaceValidation(
	UIRThermalSurfaceComponent* InTarget,
	ARadianceCaptureActor* InCapture)
{
	TargetThermalSurface = InTarget;
	RadianceCaptureActor = InCapture;
}

void AIRRadianceValidationActor::RunValidation()
{
	// La prueba localiza el pixel del objeto calcula la referencia y mide el error
	bValidationCompleted = false;
	bValidationPassed = false;
	ExpectedSensorRadiance = 0.0f;
	RenderTargetRadiance = 0.0f;
	AbsoluteError = 0.0f;
	RelativeErrorPercent = 0.0f;
	MaxRenderTargetRadiance = 0.0f;
	MaxRadianceX = 0;
	MaxRadianceY = 0;
	NonZeroPixelCount = 0;

	if (!TargetThermalSurface || !RadianceCaptureActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("IR validation needs a thermal target and RadianceCaptureActor."));
		return;
	}

	FVector TargetWorldLocation;
	if (!ResolveTarget(TargetWorldLocation, ExpectedSensorRadiance))
	{
		return;
	}

	if (bAutoSampleTargetCenter)
	{
		FIntPoint ProjectedPixel;
		if (!RadianceCaptureActor->ProjectWorldLocationToRenderTarget(
			TargetWorldLocation,
			ProjectedPixel))
		{
			UE_LOG(LogTemp, Error, TEXT("IR validation target is behind the sensor or outside the capture frame."));
			return;
		}

		SampleX = ProjectedPixel.X;
		SampleY = ProjectedPixel.Y;
	}

	RadianceCaptureActor->CaptureRadianceFrame();

	if (!ReadRenderTargetRadiance(RenderTargetRadiance))
	{
		UE_LOG(LogTemp, Warning, TEXT("IR validation could not read render target radiance."));
		return;
	}

	AbsoluteError = FMath::Abs(RenderTargetRadiance - ExpectedSensorRadiance);
	RelativeErrorPercent = ExpectedSensorRadiance > KINDA_SMALL_NUMBER
		? (AbsoluteError / ExpectedSensorRadiance) * 100.0f
		: 0.0f;
	bValidationCompleted = FMath::IsFinite(ExpectedSensorRadiance)
		&& FMath::IsFinite(RenderTargetRadiance)
		&& FMath::IsFinite(RelativeErrorPercent);
	bValidationPassed = bValidationCompleted
		&& ExpectedSensorRadiance > KINDA_SMALL_NUMBER
		&& RelativeErrorPercent <= MaxRelativeErrorPercent;

	if (bValidationPassed)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("IR validation PASSED: Expected=%f W m^-2 sr^-1, RenderTarget=%f, AbsError=%f, RelError=%f%%, Limit=%f%%"),
			ExpectedSensorRadiance,
			RenderTargetRadiance,
			AbsoluteError,
			RelativeErrorPercent,
			MaxRelativeErrorPercent);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("IR validation FAILED: Expected=%f W m^-2 sr^-1, RenderTarget=%f, AbsError=%f, RelError=%f%%, Limit=%f%%"),
			ExpectedSensorRadiance,
			RenderTargetRadiance,
			AbsoluteError,
			RelativeErrorPercent,
			MaxRelativeErrorPercent);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("IR render target diagnostics: Sample=(%d,%d), Max=%f at (%d,%d), NonZeroPixels=%d"),
			SampleX,
			SampleY,
			MaxRenderTargetRadiance,
			MaxRadianceX,
			MaxRadianceY,
			NonZeroPixelCount);
	}
}

bool AIRRadianceValidationActor::ResolveTarget(FVector& OutWorldLocation, float& OutExpectedRadiance)
{
	if (TargetThermalSurface && TargetThermalSurface->GetOwner())
	{
		TargetThermalSurface->RefreshThermalSurface();
		OutWorldLocation = TargetThermalSurface->GetOwner()->GetActorLocation();
		const float DistanceMeters = FVector::Distance(
			OutWorldLocation,
			RadianceCaptureActor->GetSensorWorldLocation()) * 0.01f;
		OutExpectedRadiance = TargetThermalSurface->GetCurrentSensorRadiance(DistanceMeters);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("IR validation thermal target is not valid."));
	return false;
}

bool AIRRadianceValidationActor::ReadRenderTargetRadiance(float& OutRadiance)
{
	UTextureRenderTarget2D* RenderTarget = RadianceCaptureActor
		? RadianceCaptureActor->GetRadianceRenderTarget()
		: nullptr;
	if (!RenderTarget)
	{
		return false;
	}

	FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!Resource)
	{
		return false;
	}

	TArray<FFloat16Color> Pixels;
	if (!Resource->ReadFloat16Pixels(Pixels))
	{
		return false;
	}

	const int32 X = FMath::Clamp(SampleX, 0, RenderTarget->SizeX - 1);
	const int32 Y = FMath::Clamp(SampleY, 0, RenderTarget->SizeY - 1);
	const int32 Index = Y * RenderTarget->SizeX + X;
	if (!Pixels.IsValidIndex(Index))
	{
		return false;
	}

	for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
	{
		const float Radiance = Pixels[PixelIndex].R.GetFloat();
		if (FMath::Abs(Radiance) > KINDA_SMALL_NUMBER)
		{
			++NonZeroPixelCount;
		}

		if (Radiance > MaxRenderTargetRadiance)
		{
			MaxRenderTargetRadiance = Radiance;
			MaxRadianceX = PixelIndex % RenderTarget->SizeX;
			MaxRadianceY = PixelIndex / RenderTarget->SizeX;
		}
	}

	OutRadiance = Pixels[Index].R.GetFloat();
	return true;
}
