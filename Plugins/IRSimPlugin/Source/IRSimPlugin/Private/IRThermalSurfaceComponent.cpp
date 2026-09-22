// Copyright Epic Games, Inc. All Rights Reserved.

#include "IRThermalSurfaceComponent.h"

#include "Components/StaticMeshComponent.h"
#include "IRCoreBridge.h"
#include "IRSceneEnvironmentActor.h"
#include "ThermalPipelineController.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// Componente reutilizable para aplicar un estado termico a una malla existente
// Mantiene la configuracion y prepara los datos que consume el material

namespace
{
	//ids de los Custom Primitive Data que recibe el material fisico
	constexpr int32 CpdTemperatureK = 0;
	constexpr int32 CpdAirTemperatureK = 3;
	constexpr int32 CpdBandMinMicrons = 6;
	constexpr int32 CpdBandMaxMicrons = 7;
	// El material M_ThermalSurface toma L_surface de este índice antes de
	// aplicar Beer-Lambert. Debe contener la radiancia de cuerpo negro de la
	// superficie; el shader aplica después la emisividad direccional Fresnel.
	constexpr int32 CpdSurfaceBandRadiance = 8;
	constexpr int32 CpdAirBandRadiance = 9;
	constexpr int32 CpdAtmosphericExtinction = 10;
	constexpr int32 CpdObjectBlackbodyBandRadiance = 16;
	constexpr int32 CpdMaterialId = 18;
	constexpr int32 CpdSkyHorizonTemperatureK = 19;
	constexpr int32 CpdSkyZenithTemperatureK = 20;
	constexpr int32 CpdComplexRefractiveIndexReal = 21;
	constexpr int32 CpdComplexRefractiveIndexImaginary = 22;
	constexpr int32 CpdSkyHorizonBandRadiance = 23;
	constexpr int32 CpdSkyZenithBandRadiance = 24;
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

	// En mapas con World Partition una malla puede cargarse después del
	// controlador. Recuperamos tanto el entorno como el material físico al
	// activarse; antes solo se actualizaban los datos y la malla seguía usando
	// su material original.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AIRSceneEnvironmentActor> It(World); It; ++It)
		{
			ApplySceneEnvironment(*It);
			break;
		}

		for (TActorIterator<AThermalPipelineController> It(World); It; ++It)
		{
			if (It->ShouldAutoAssignThermalMaterial() && It->GetDefaultThermalMaterial())
			{
				SetDebugMaterial(It->GetDefaultThermalMaterial());
			}
			break;
		}
	}

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
	SkyHorizonTemperatureK = SceneEnvironment->GetSkyHorizonTemperatureK();
	SkyZenithTemperatureK = SceneEnvironment->GetSkyZenithTemperatureK();
	AtmosphericExtinctionCoefficient = SceneEnvironment->GetAtmosphericExtinctionCoefficient();
	BandMinMicrons = SceneEnvironment->GetBandMinMicrons();
	BandMaxMicrons = SceneEnvironment->GetBandMaxMicrons();
	SpectralIntegrationSamples = SceneEnvironment->GetSpectralIntegrationSamples();
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetTemperatureKelvin(float InTemperatureK)
{
	TemperatureK = FMath::Max(InTemperatureK, 0.0f);
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetTargetMesh(UStaticMeshComponent* InTargetMesh)
{
	TargetMesh = InTargetMesh;
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetComplexRefractiveIndex(float InRealPart, float InImaginaryPart)
{
	ComplexRefractiveIndexReal = FMath::Max(InRealPart, 0.0f);
	ComplexRefractiveIndexImaginary = FMath::Max(InImaginaryPart, 0.0f);
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetAtmosphericExtinctionCoefficient(float InCoefficient)
{
	AtmosphericExtinctionCoefficient = FMath::Max(InCoefficient, 0.0f);
	RefreshThermalSurface();
}

void UIRThermalSurfaceComponent::SetDebugMaterial(UMaterialInterface* InDebugMaterial)
{
	DebugMaterial = InDebugMaterial;
	DynamicDebugMaterial = nullptr;
	RefreshThermalSurface();
}

float UIRThermalSurfaceComponent::GetCurrentBlackbodyBandRadiance() const
{
	return irsim::core::ComputeBandRadiance(
		TemperatureK,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float UIRThermalSurfaceComponent::GetCurrentAirBandRadiance() const
{
	return irsim::core::ComputeBandRadiance(
		AirTemperatureK,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
}

float UIRThermalSurfaceComponent::GetCurrentBlackbodySensorRadiance(float DistanceMeters) const
{
	// Reference only for the blackbody fixture (n=1, k=0). The production
	// material evaluates directional Fresnel emissivity per pixel on the GPU.
	const float TauFromDistance = irsim::core::ComputeAtmosphericTransmittance(
		AtmosphericExtinctionCoefficient,
		FMath::Max(DistanceMeters, 0.0f));
	return irsim::core::ComputeSensorBandRadiance(
		GetCurrentBlackbodyBandRadiance(),
		GetCurrentAirBandRadiance(),
		TauFromDistance);
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
			for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
			{
				Mesh->SetMaterial(MaterialIndex, DynamicDebugMaterial);
			}
		}
	}

	const float ObjectBlackbodyBandRadiance = irsim::core::ComputeBandRadiance(
		TemperatureK,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	const float SkyHorizonBandRadiance = irsim::core::ComputeBandRadiance(
		SkyHorizonTemperatureK,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	const float SkyZenithBandRadiance = irsim::core::ComputeBandRadiance(
		SkyZenithTemperatureK,
		BandMinMicrons,
		BandMaxMicrons,
		SpectralIntegrationSamples);
	const float AirRadiance = GetCurrentAirBandRadiance();

	if (DynamicDebugMaterial)
	{
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("TemperatureK"), TemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("ObjectBlackbodyBandRadiance"), ObjectBlackbodyBandRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirTemperatureK"), AirTemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyHorizonTemperatureK"), SkyHorizonTemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyZenithTemperatureK"), SkyZenithTemperatureK);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("ComplexRefractiveIndexReal"), ComplexRefractiveIndexReal);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("ComplexRefractiveIndexImaginary"), ComplexRefractiveIndexImaginary);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyHorizonBandRadiance"), SkyHorizonBandRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("SkyZenithBandRadiance"), SkyZenithBandRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AirRadiance"), AirRadiance);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMinMicrons"), BandMinMicrons);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("BandMaxMicrons"), BandMaxMicrons);
		DynamicDebugMaterial->SetScalarParameterValue(TEXT("AtmosphericExtinction"), AtmosphericExtinctionCoefficient);
	}

	Mesh->SetCustomPrimitiveDataFloat(CpdTemperatureK, TemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdAirTemperatureK, AirTemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdBandMinMicrons, BandMinMicrons);
	Mesh->SetCustomPrimitiveDataFloat(CpdBandMaxMicrons, BandMaxMicrons);
	Mesh->SetCustomPrimitiveDataFloat(CpdSurfaceBandRadiance, ObjectBlackbodyBandRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdAirBandRadiance, AirRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdAtmosphericExtinction, AtmosphericExtinctionCoefficient);
	Mesh->SetCustomPrimitiveDataFloat(CpdObjectBlackbodyBandRadiance, ObjectBlackbodyBandRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdMaterialId, static_cast<float>(MaterialId));
	Mesh->SetCustomPrimitiveDataFloat(CpdSkyHorizonTemperatureK, SkyHorizonTemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdSkyZenithTemperatureK, SkyZenithTemperatureK);
	Mesh->SetCustomPrimitiveDataFloat(CpdComplexRefractiveIndexReal, ComplexRefractiveIndexReal);
	Mesh->SetCustomPrimitiveDataFloat(CpdComplexRefractiveIndexImaginary, ComplexRefractiveIndexImaginary);
	Mesh->SetCustomPrimitiveDataFloat(CpdSkyHorizonBandRadiance, SkyHorizonBandRadiance);
	Mesh->SetCustomPrimitiveDataFloat(CpdSkyZenithBandRadiance, SkyZenithBandRadiance);
	Mesh->MarkRenderStateDirty();
}
