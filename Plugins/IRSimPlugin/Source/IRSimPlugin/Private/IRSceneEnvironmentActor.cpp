// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRSceneEnvironmentActor.h"

#include "EngineUtils.h"
#include "IRThermalSurfaceComponent.h"

// Contenedor de parametros ambientales comunes a toda la escena termica

AIRSceneEnvironmentActor::AIRSceneEnvironmentActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AIRSceneEnvironmentActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyToThermalSurfaces();
}

void AIRSceneEnvironmentActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyToThermalSurfaces();
}

void AIRSceneEnvironmentActor::SetAirTemperatureKelvin(float InTemperatureK)
{
	AirTemperatureK = FMath::Max(InTemperatureK, 0.0f);
	ApplyToThermalSurfaces();
}

void AIRSceneEnvironmentActor::SetAtmosphericExtinctionCoefficient(float InCoefficient)
{
	AtmosphericExtinctionCoefficient = FMath::Max(InCoefficient, 0.0f);
	ApplyToThermalSurfaces();
}

void AIRSceneEnvironmentActor::SetSkyTemperaturesKelvin(
	float InHorizonK, float InZenithK)
{
	SkyHorizonTemperatureK = FMath::Max(InHorizonK, 0.0f);
	SkyZenithTemperatureK = FMath::Max(InZenithK, 0.0f);
	ApplyToThermalSurfaces();
}

void AIRSceneEnvironmentActor::SetSpectralBand(
	float InBandMinMicrons, float InBandMaxMicrons, int32 InIntegrationSamples)
{
	BandMinMicrons = FMath::Max(InBandMinMicrons, 0.1f);
	BandMaxMicrons = FMath::Max(InBandMaxMicrons, BandMinMicrons + 0.1f);
	SpectralIntegrationSamples = FMath::Max(InIntegrationSamples, 4);
	ApplyToThermalSurfaces();
}

void AIRSceneEnvironmentActor::ApplyToThermalSurfaces()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TInlineComponentArray<UIRThermalSurfaceComponent*> ThermalSurfaces(*It);
		for (UIRThermalSurfaceComponent* ThermalSurface : ThermalSurfaces)
		{
			ThermalSurface->ApplySceneEnvironment(this);
		}
	}
}
