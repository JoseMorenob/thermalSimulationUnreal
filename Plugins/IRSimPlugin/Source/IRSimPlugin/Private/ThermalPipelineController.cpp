// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThermalPipelineController.h"

#include "EngineUtils.h"
#include "IRSceneEnvironmentActor.h"
#include "IRThermalSurfaceComponent.h"
#include "Materials/MaterialInterface.h"
#include "RadianceCaptureActor.h"

// Coordina el orden de actualizacion del entorno los objetos y la captura

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
	// Primero se propaga el contexto y despues se actualizan materiales y captura
	ApplySceneEnvironmentToActors();
	ApplyThermalMaterialToActors();
	RefreshCaptureActor();

	if (bCaptureAfterRefresh)
	{
		CaptureRadianceNow();
	}
}

void AThermalPipelineController::CaptureRadianceNow()
{
	ApplySceneEnvironmentToActors();

	if (RadianceCaptureActor)
	{
		RadianceCaptureActor->CaptureRadianceFrame();
	}
}

void AThermalPipelineController::ApplySceneEnvironmentToActors()
{
	// Todos los objetos deben compartir aire cielo banda y atmosfera
	if (!SceneEnvironmentActor || !GetWorld())
	{
		return;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UIRThermalSurfaceComponent*> ThermalSurfaces(*It);
		for (UIRThermalSurfaceComponent* ThermalSurface : ThermalSurfaces)
		{
			ThermalSurface->ApplySceneEnvironment(SceneEnvironmentActor);
		}
	}
}

void AThermalPipelineController::ApplyThermalMaterialToActors()
{
	if (!bAutoAssignMaterialToThermalActors || !DefaultThermalMaterial || !GetWorld())
	{
		return;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UIRThermalSurfaceComponent*> ThermalSurfaces(*It);
		for (UIRThermalSurfaceComponent* ThermalSurface : ThermalSurfaces)
		{
			ThermalSurface->SetDebugMaterial(DefaultThermalMaterial);
		}
	}
}

void AThermalPipelineController::RefreshCaptureActor()
{
	if (RadianceCaptureActor)
	{
		RadianceCaptureActor->RefreshCapturePipeline();
	}
}
