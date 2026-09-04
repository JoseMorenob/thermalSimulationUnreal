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
	PrimaryActorTick.bCanEverTick = true;
}

void AThermalPipelineController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!SceneEnvironmentActor || !GetWorld())
	{
		return;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UIRThermalSurfaceComponent*> ThermalSurfaces(*It);
		for (UIRThermalSurfaceComponent* ThermalSurface : ThermalSurfaces)
		{
			ThermalSurface->AdvanceThermalState(
				DeltaSeconds,
				SceneEnvironmentActor->IsThermalDynamicsEnabled(),
				SceneEnvironmentActor->GetSolarIrradianceWm2(),
				SceneEnvironmentActor->GetAirTemperatureK(),
				SceneEnvironmentActor->GetEffectiveSkyTemperatureK());
		}
	}

	ApplyThermalMaterialToActors();
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
	UpdateThermalActorsSensorLocation();
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
	UpdateThermalActorsSensorLocation();

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

void AThermalPipelineController::UpdateThermalActorsSensorLocation()
{
	// La distancia atmosferica se mide desde el sensor virtual y no desde el editor
	if (!RadianceCaptureActor || !GetWorld())
	{
		return;
	}

	const FVector RadianceSensorLocation = RadianceCaptureActor->GetSensorWorldLocation();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UIRThermalSurfaceComponent*> ThermalSurfaces(*It);
		for (UIRThermalSurfaceComponent* ThermalSurface : ThermalSurfaces)
		{
			ThermalSurface->SetRadianceSensorWorldLocation(RadianceSensorLocation);
		}
	}
}

void AThermalPipelineController::ApplyThermalMaterialToActors()
{
	// Se fuerza el material fisico para que la captura no dependa de la vista debug
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
			ThermalSurface->RefreshThermalSurface();
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
