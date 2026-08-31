// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRThermalSurfaceComponent.h"

#include "Components/StaticMeshComponent.h"
#include "IRCoreBridge.h"
#include "IRSceneEnvironmentActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// Componente reutilizable para aplicar un estado termico a una malla existente
// Mantiene la configuracion y prepara los datos que consume el material

namespace
{
	//ids de los Custom Primitive Data que recibe el material fisico
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
	constexpr int32 CpdDirectionalEmissivityFront = 12;
	constexpr int32 CpdDirectionalEmissivityGrazing = 13;
	constexpr int32 CpdDirectionalAngularFalloffPower = 14;
	constexpr int32 CpdUseDirectionalEmissivity = 15;
	constexpr int32 CpdObjectBlackbodyBandRadiance = 16;
	constexpr int32 CpdSkyBlackbodyBandRadiance = 17;
}

UIRThermalSurfaceComponent::UIRThermalSurfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIRThermalSurfaceComponent::OnRegister()
{
	Super::OnRegister();
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::RefreshThermalSurface()
{
	// Este es el punto unico de actualizacion despues de cambiar una propiedad
	PushThermalDataToPrimitive();
}

void UIRThermalSurfaceComponent::ApplySceneEnvironment(const AIRSceneEnvironmentActor* SceneEnvironment)
{
	// Copiamos el contexto comun para que la superficie use la misma atmosfera
	if (!SceneEnvironment)
	{
		return;
	}

	AirTemperatureK = SceneEnvironment->GetAirTemperatureK();
	EffectiveSkyTemperatureK = SceneEnvironment->GetEffectiveSkyTemperatureK();
	AtmosphericTransmittance = SceneEnvironment->GetAtmosphericTransmittance();
	AtmosphericExtinctionCoefficient = SceneEnvironment->GetAtmosphericExtinctionCoefficient();
	BandMinMicrons = SceneEnvironment->GetBandMinMicrons();
	BandMaxMicrons = SceneEnvironment->GetBandMaxMicrons();
	SpectralIntegrationSamples = SceneEnvironment->GetSpectralIntegrationSamples();
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetRadianceSensorWorldLocation(const FVector& WorldLocation)
{
	RadianceSensorWorldLocation = WorldLocation;
	bHasRadianceSensorWorldLocation = true;
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetTemperatureKelvin(float InTemperatureK)
{
	TemperatureK = FMath::Max(InTemperatureK, 0.0f);
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetDebugMaterial(UMaterialInterface* InDebugMaterial)
{
	DebugMaterial = InDebugMaterial;
	DynamicDebugMaterial = nullptr;
	RefreshThermalSurface();
}

float UIRThermalSurfaceComponent::GetCurrentSurfaceBandRadiance() const
{
	// Calculamos la radiancia que abandona la superficie antes de la atmosfera
	return irsim::core::ComputeSurfaceBandRadiance(
		TemperatureK,
		EffectiveSkyTemperatureK,
		Emissivity,
		Transmissivity,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float UIRThermalSurfaceComponent::GetCurrentAirBandRadiance() const
{
	return irsim::core::ComputeBandRadiance(
		AirTemperatureK,
		1.0f,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float UIRThermalSurfaceComponent::GetCurrentSensorRadiance(float DistanceMeters) const
{
	// Aplicamos la perdida atmosferica usando la distancia hasta el sensor virtual
	const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
		AtmosphericExtinctionCoefficient,
		FMath::Max(DistanceMeters, 0.0f));
	const float EffectiveTau = FMath::Clamp(AtmosphericTransmittance * TauFromDistance, 0.0f, 1.0f);
	return irsim::core::ComputeSensorBandRadiance(
		GetCurrentSurfaceBandRadiance(),
		GetCurrentAirBandRadiance(),
		EffectiveTau);
}

UStaticMeshComponent* UIRThermalSurfaceComponent::ResolveTargetMesh() const
{
	if (TargetMesh)
	{
		return TargetMesh;
	}

	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UStaticMeshComponent>() : nullptr;
}

float UIRThermalSurfaceComponent::GetSensorDistanceMeters() const
{
	const UStaticMeshComponent* Mesh = ResolveTargetMesh();
	if (!bHasRadianceSensorWorldLocation || !Mesh)
	{
		return 0.0f;
	}

	return FVector::Distance(Mesh->GetComponentLocation(), RadianceSensorWorldLocation) * 0.01f;
}

void UIRThermalSurfaceComponent::PushThermalDataToPrimitive()
{
	// El material recibe los resultados mediante Custom Primitive Data por malla
	UStaticMeshComponent* Mesh = ResolveTargetMesh();
	if (!Mesh)
	{
		return;
	}

	if (DebugMaterial)
	{
		if (!DynamicDebugMaterial || DynamicDebugMaterial->Parent != DebugMaterial)
		{
			DynamicDebugMaterial = UMaterialInstanceDynamic::Create(DebugMaterial, this);
			Mesh->SetMaterial(0, DynamicDebugMaterial);
		}
	}

	const float Reflectivity = FMath::Max(0.0f, 1.0f - Emissivity - Transmissivity);
	const float ObjectBlackbodyBandRadiance = irsim::core::ComputeBandRadiance(
		TemperatureK,
		1.0f,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	const float SkyBlackbodyBandRadiance = irsim::core::ComputeBandRadiance(
		EffectiveSkyTemperatureK,
		1.0f,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	const float SurfaceRadiance =
		ObjectBlackbodyBandRadiance * Emissivity + SkyBlackbodyBandRadiance * Reflectivity;
	const float AirRadiance = GetCurrentAirBandRadiance();
	const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
		AtmosphericExtinctionCoefficient,
		GetSensorDistanceMeters());
	const float AtmosphericTau = FMath::Clamp(AtmosphericTransmittance * TauFromDistance, 0.0f, 1.0f);
	const float SensorRadiance = irsim::core::ComputeSensorBandRadiance(
		SurfaceRadiance,
		AirRadiance,
		AtmosphericTau);

	if (DynamicDebugMaterial)
	{
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("TemperatureK"), TemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("Emissivity"), Emissivity);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("UseDirectionalEmissivity"), bUseDirectionalEmissivity ? 1.0f : 0.0f);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("DirectionalEmissivityFront"), DirectionalEmissivityFront);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("DirectionalEmissivityGrazing"), DirectionalEmissivityGrazing);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("DirectionalAngularFalloffPower"), DirectionalAngularFalloffPower);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("Reflectivity"), Reflectivity);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("ObjectBlackbodyBandRadiance"), ObjectBlackbodyBandRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyBlackbodyBandRadiance"), SkyBlackbodyBandRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("Transmissivity"), Transmissivity);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirTemperatureK"), AirTemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyTemperatureK"), EffectiveSkyTemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AtmosphericTau"), AtmosphericTau);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SurfaceRadiance"), SurfaceRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SensorRadiance"), SensorRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirRadiance"), AirRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMinMicrons"), BandMinMicrons);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMaxMicrons"), BandMaxMicrons);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AtmosphericExtinction"), AtmosphericExtinctionCoefficient);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("RadianceNormalizationMax"), RadianceNormalizationMax);
	}

	Mesh->SetCustomPrimitiveDataFloat(CpdTemperatureK, TemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdEmissivity, Emissivity);
	Mesh->SetCustomPrimitiveDataFloat(CpdTransmissivity, Transmissivity);
	Mesh->SetCustomPrimitiveDataFloat(CpdAirTemperatureK, AirTemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdSkyTemperatureK, EffectiveSkyTemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdAtmosphericBaseTransmittance, AtmosphericTransmittance);
	Mesh->SetCustomPrimitiveDataFloat(CpdBandMinMicrons, BandMinMicrons);
	Mesh->SetCustomPrimitiveDataFloat(CpdBandMaxMicrons, BandMaxMicrons);
	Mesh->SetCustomPrimitiveDataFloat(CpdSurfaceBandRadiance, SurfaceRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdAirBandRadiance, AirRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdAtmosphericExtinction, AtmosphericExtinctionCoefficient);
	Mesh->SetCustomPrimitiveDataFloat(CpdRadianceNormalizationMax, RadianceNormalizationMax);
	Mesh->SetCustomPrimitiveDataFloat(CpdDirectionalEmissivityFront, DirectionalEmissivityFront);
	Mesh->SetCustomPrimitiveDataFloat(CpdDirectionalEmissivityGrazing, DirectionalEmissivityGrazing);
	Mesh->SetCustomPrimitiveDataFloat(CpdDirectionalAngularFalloffPower, DirectionalAngularFalloffPower);
	Mesh->SetCustomPrimitiveDataFloat(CpdUseDirectionalEmissivity, bUseDirectionalEmissivity ? 1.0f : 0.0f);
	Mesh->SetCustomPrimitiveDataFloat(CpdObjectBlackbodyBandRadiance, ObjectBlackbodyBandRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdSkyBlackbodyBandRadiance, SkyBlackbodyBandRadiance);
	Mesh->MarkRenderStateDirty();
}
