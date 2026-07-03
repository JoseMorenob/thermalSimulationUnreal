// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThermalCubeActor.h"

#include "Components/StaticMeshComponent.h"
#include "IRCoreBridge.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AThermalCubeActor::AThermalCubeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	SetRootComponent(CubeMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		CubeMesh->SetStaticMesh(CubeAsset.Object);
	}
}

void AThermalCubeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PushThermalDataToPrimitive();
}

void AThermalCubeActor::BeginPlay()
{
	Super::BeginPlay();
	PushThermalDataToPrimitive();
}

float AThermalCubeActor::GetObjectDistanceTo(const FVector& WorldLocation) const
{
	return FVector::Distance(GetActorLocation(), WorldLocation);
}

float AThermalCubeActor::GetCurrentEmittedRadiance() const
{
	return irsim::core::ComputeBandRadiance(
		TemperatureK,
		Emissivity,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float AThermalCubeActor::GetCurrentSensorRadiance(float DistanceMeters) const
{
	const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
		AtmosphericExtinctionCoefficient,
		FMath::Max(DistanceMeters, 0.0f));
	const float EffectiveTau = FMath::Clamp(AtmosphericTransmittance * TauFromDistance, 0.0f, 1.0f);
	const float SurfaceRadiance = irsim::core::ComputeSurfaceBandRadiance(
		TemperatureK,
		EffectiveSkyTemperatureK,
		Emissivity,
		Transmissivity,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	const float AirRadiance = irsim::core::ComputeBandRadiance(
		AirTemperatureK,
		1.0f,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	return irsim::core::ComputeSensorBandRadiance(
		SurfaceRadiance,
		AirRadiance,
		EffectiveTau);
}

void AThermalCubeActor::SetDebugMaterial(UMaterialInterface* InDebugMaterial)
{
	DebugMaterial = InDebugMaterial;
	DynamicDebugMaterial = nullptr;
	PushThermalDataToPrimitive();
}

void AThermalCubeActor::RefreshThermalMaterial()
{
	PushThermalDataToPrimitive();
}

void AThermalCubeActor::PushThermalDataToPrimitive()
{
	if (!CubeMesh)
	{
		return;
	}

	if (DebugMaterial)
	{
		if (!DynamicDebugMaterial || DynamicDebugMaterial->Parent != DebugMaterial)
		{
			DynamicDebugMaterial = UMaterialInstanceDynamic::Create(DebugMaterial, this);
			CubeMesh->SetMaterial(0, DynamicDebugMaterial);
		}

		if (DynamicDebugMaterial)
		{
			// Este bloque solo prepara la visualizacion de depuracion;
			// la salida fisica util sigue siendo la radiancia en banda.
			const float Reflectivity = FMath::Max(0.0f, 1.0f - Emissivity - Transmissivity);
			const float EmittedRadiance = irsim::core::ComputeBandRadiance(
				TemperatureK,
				Emissivity,
				BandMinMicrons,
				BandMaxMicrons,
				SpectralIntegrationSamples);
			const float ReflectedRadiance = irsim::core::ComputeBandRadiance(
				EffectiveSkyTemperatureK,
				Reflectivity,
				BandMinMicrons,
				BandMaxMicrons,
				SpectralIntegrationSamples);
			const float AirRadiance = irsim::core::ComputeBandRadiance(
				AirTemperatureK,
				1.0f,
				BandMinMicrons,
				BandMaxMicrons,
				SpectralIntegrationSamples);
			const float CameraDistanceMeters =
				(GetWorld() && GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
				? GetObjectDistanceTo(GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation()) * 0.01f
				: 0.0f;
			const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
				AtmosphericExtinctionCoefficient,
				CameraDistanceMeters);
			const float EffectiveTau = FMath::Clamp(AtmosphericTransmittance * TauFromDistance, 0.0f, 1.0f);
			const float SensorRadiance = irsim::core::ComputeSensorBandRadiance(
				EmittedRadiance + ReflectedRadiance,
				AirRadiance,
				EffectiveTau);

			DynamicDebugMaterial->SetScalarParameterValue(TEXT("TemperatureK"), TemperatureK);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("Emissivity"), Emissivity);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("Reflectivity"), Reflectivity);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("Transmissivity"), Transmissivity);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirTemperatureK"), AirTemperatureK);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyTemperatureK"), EffectiveSkyTemperatureK);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("AtmosphericTau"), EffectiveTau);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("EmittedRadiance"), EmittedRadiance);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("ReflectedRadiance"), ReflectedRadiance);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("SensorRadiance"), SensorRadiance);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirRadiance"), AirRadiance);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMinMicrons"), BandMinMicrons);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMaxMicrons"), BandMaxMicrons);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("AtmosphericExtinction"), AtmosphericExtinctionCoefficient);
			DynamicDebugMaterial->SetScalarParameterValue(TEXT("RadianceNormalizationMax"), RadianceNormalizationMax);
		}
	}

	// Estos Custom Primitive Data dejan el material alineado con el pipeline
	// simplificado del documento: temperatura -> radiancia -> atmosfera -> captura.
	CubeMesh->SetCustomPrimitiveDataFloat(0, TemperatureK);
	CubeMesh->SetCustomPrimitiveDataFloat(1, Emissivity);
	CubeMesh->SetCustomPrimitiveDataFloat(2, Transmissivity);
	CubeMesh->SetCustomPrimitiveDataFloat(3, AirTemperatureK);
	CubeMesh->SetCustomPrimitiveDataFloat(4, EffectiveSkyTemperatureK);
	CubeMesh->SetCustomPrimitiveDataFloat(5, AtmosphericTransmittance);
	CubeMesh->SetCustomPrimitiveDataFloat(6, BandMinMicrons);
	CubeMesh->SetCustomPrimitiveDataFloat(7, BandMaxMicrons);
	CubeMesh->MarkRenderStateDirty();
}
