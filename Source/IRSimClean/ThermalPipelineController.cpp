// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThermalPipelineController.h"

#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "RadianceCaptureActor.h"
#include "ThermalCubeActor.h"

AThermalPipelineController::AThermalPipelineController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AThermalPipelineController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPipeline();
}

void AThermalPipelineController::BeginPlay()
{
	Super::BeginPlay();
	RefreshPipeline();
}

void AThermalPipelineController::RefreshPipeline()
{
	ApplyThermalMaterialToActors();
	RefreshCaptureActor();

	if (bCaptureAfterRefresh)
	{
		CaptureRadianceNow();
	}
}

void AThermalPipelineController::CaptureRadianceNow()
{
	if (RadianceCaptureActor)
	{
		RadianceCaptureActor->CaptureRadianceFrame();
	}
}

void AThermalPipelineController::ApplyThermalMaterialToActors()
{
	if (!bAutoAssignMaterialToThermalActors || !DefaultThermalMaterial || !GetWorld())
	{
		return;
	}

	for (TActorIterator<AThermalCubeActor> It(GetWorld()); It; ++It)
	{
		AThermalCubeActor* ThermalActor = *It;
		if (!ThermalActor)
		{
			continue;
		}

		ThermalActor->SetDebugMaterial(DefaultThermalMaterial);
		ThermalActor->RefreshThermalMaterial();
	}
}

void AThermalPipelineController::RefreshCaptureActor()
{
	if (RadianceCaptureActor)
	{
		RadianceCaptureActor->RefreshCapturePipeline();
	}
}
