// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThermalCubeActor.h"

#include "Components/StaticMeshComponent.h"
#include "IRCoreBridge.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 CpdTemperatureK = 0;
	constexpr int32 CpdEmissivity = 1;
	constexpr int32 CpdTransmissivity = 2;
	constexpr int32 CpdAirTemperatureK = 3;
	constexpr int32 CpdSkyTemperatureK = 4;
	constexpr int32 CpdAtmosphericBaseTransmittance = 5;
	constexpr int32 CpdBandMinMicrons = 6;
	constexpr int32 CpdBandMaxMicrons = 7;
	constexpr int32 CpdSurfaceBandRadiance = 8;
	constexpr int32 CpdAirBandRadiance = 9;
	constexpr int32 CpdAtmosphericExtinction = 10;
	constexpr int32 CpdRadianceNormalizationMax = 11;
}

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

float AThermalCubeActor::GetCurrentSurfaceBandRadiance() const
{
	return irsim::core::ComputeSurfaceBandRadiance(
		TemperatureK,
		EffectiveSkyTemperatureK,
		Emissivity,
		Transmissivity,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float AThermalCubeActor::GetCurrentAirBandRadiance() const
{
	return irsim::core::ComputeBandRadiance(
		AirTemperatureK,
		1.0f,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float AThermalCubeActor::GetCurrentSensorRadiance(float DistanceMeters) const
{
	// Beer-Lambert atmospheric attenuation, consistent with the simplified IR pipeline audit.
	const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
		AtmosphericExtinctionCoefficient,
		FMath::Max(DistanceMeters, 0.0f));
	const float EffectiveTau = FMath::Clamp(AtmosphericTransmittance * TauFromDistance, 0.0f, 1.0f);
	return irsim::core::ComputeSensorBandRadiance(
		GetCurrentSurfaceBandRadiance(),
		GetCurrentAirBandRadiance(),
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

void AThermalCubeActor::SetRadianceSensorWorldLocation(const FVector& WorldLocation)
{
	RadianceSensorWorldLocation = WorldLocation;
	bHasRadianceSensorWorldLocation = true;
	PushThermalDataToPrimitive();
}

float AThermalCubeActor::GetSensorDistanceMeters() const
{
	if (!bHasRadianceSensorWorldLocation)
	{
		return 0.0f;
	}

	return GetObjectDistanceTo(RadianceSensorWorldLocation) * 0.01f;
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
	}

	if (DynamicDebugMaterial)
	{
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
		const float SurfaceRadiance = EmittedRadiance + ReflectedRadiance;
		const float SensorDistanceMeters = GetSensorDistanceMeters();
		// Beer-Lambert atmospheric attenuation, using the virtual IR capture as sensor.
		const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
			AtmosphericExtinctionCoefficient,
			SensorDistanceMeters);
		const float EffectiveTau = FMath::Clamp(AtmosphericTransmittance * TauFromDistance, 0.0f, 1.0f);
		const float SensorRadiance = irsim::core::ComputeSensorBandRadiance(
			SurfaceRadiance,
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
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SurfaceRadiance"), SurfaceRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SensorRadiance"), SensorRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirRadiance"), AirRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMinMicrons"), BandMinMicrons);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMaxMicrons"), BandMaxMicrons);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AtmosphericExtinction"), AtmosphericExtinctionCoefficient);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("RadianceNormalizationMax"), RadianceNormalizationMax);
	}

	CubeMesh->SetCustomPrimitiveDataFloat(CpdTemperatureK, TemperatureK);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdEmissivity, Emissivity);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdTransmissivity, Transmissivity);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdAirTemperatureK, AirTemperatureK);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdSkyTemperatureK, EffectiveSkyTemperatureK);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdAtmosphericBaseTransmittance, AtmosphericTransmittance);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdBandMinMicrons, BandMinMicrons);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdBandMaxMicrons, BandMaxMicrons);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdSurfaceBandRadiance, GetCurrentSurfaceBandRadiance());
	CubeMesh->SetCustomPrimitiveDataFloat(CpdAirBandRadiance, GetCurrentAirBandRadiance());
	CubeMesh->SetCustomPrimitiveDataFloat(CpdAtmosphericExtinction, AtmosphericExtinctionCoefficient);
	CubeMesh->SetCustomPrimitiveDataFloat(CpdRadianceNormalizationMax, RadianceNormalizationMax);
	CubeMesh->MarkRenderStateDirty();
}
