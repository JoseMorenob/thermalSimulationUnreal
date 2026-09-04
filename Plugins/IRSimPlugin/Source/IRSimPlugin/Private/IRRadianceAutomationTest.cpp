// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

// Test de integracion CPU frente a GPU en una escena minima controlada.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "IRThermalSurfaceComponent.h"
#include "IRRadianceValidationActor.h"
#include "Materials/MaterialInterface.h"
#include "RadianceCaptureActor.h"
#include "RenderingThread.h"

// Escena minima automatica para comprobar el recorrido completo de los datos
// desde el modelo fisico hasta la textura de salida

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIRRadianceCpuGpuMinimalSceneTest,
	"IRSimClean.Radiance.CpuGpuMinimalScene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIRRadianceCpuGpuMinimalSceneTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddError(TEXT("GEngine is not available."));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TEXT("IRRadianceAutomationWorld"),
		GetTransientPackage());
	if (!World)
	{
		AddError(TEXT("Could not create the transient validation world."));
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	auto DestroyTestWorld = [World]()
	{
		GEngine->ShutdownWorldNetDriver(World);
		World->DestroyWorld(true);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};

	UMaterialInterface* ThermalMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/IRSimPlugin/Materials/M_ThermalSurface.M_ThermalSurface"));
	if (!ThermalMaterial)
	{
		AddError(TEXT("Could not load M_ThermalSurface."));
		DestroyTestWorld();
		return false;
	}

	ARadianceCaptureActor* CaptureActor = World->SpawnActor<ARadianceCaptureActor>();
	AStaticMeshActor* GenericMeshActor = World->SpawnActor<AStaticMeshActor>();
	AIRRadianceValidationActor* ValidationActor = World->SpawnActor<AIRRadianceValidationActor>();
	ADirectionalLight* DirectionalLight = World->SpawnActor<ADirectionalLight>();
	if (!CaptureActor || !GenericMeshActor || !ValidationActor || !DirectionalLight)
	{
		AddError(TEXT("Could not spawn the IR validation actors."));
		DestroyTestWorld();
		return false;
	}

	CaptureActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	GenericMeshActor->SetActorLocation(FVector(1000.0, 0.0, 0.0));
	GenericMeshActor->SetActorScale3D(FVector(5.0));
	UStaticMeshComponent* GenericMesh = GenericMeshActor->GetStaticMeshComponent();
	if (!GenericMesh)
	{
		AddError(TEXT("Could not resolve the validation mesh components."));
		DestroyTestWorld();
		return false;
	}
	UStaticMesh* CubeAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeAsset)
	{
		AddError(TEXT("Could not load the validation mesh asset."));
		DestroyTestWorld();
		return false;
	}
	GenericMesh->SetStaticMesh(CubeAsset);
	UIRThermalSurfaceComponent* GenericThermalSurface = NewObject<UIRThermalSurfaceComponent>(GenericMeshActor);
	GenericMeshActor->AddInstanceComponent(GenericThermalSurface);
	GenericThermalSurface->RegisterComponent();
	DirectionalLight->SetActorRotation(FRotator(-45.0, 0.0, 0.0));
	GenericThermalSurface->SetDebugMaterial(ThermalMaterial);
	GenericThermalSurface->SetEmissivity(1.0f);
	GenericThermalSurface->SetAtmosphericExtinctionCoefficient(0.0f);
	ValidationActor->ConfigureSurfaceValidation(GenericThermalSurface, CaptureActor);

	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->UpdateWorldComponents(true, false);
	FlushRenderingCommands();

	constexpr float TestTemperaturesK[] = { 280.0f, 300.0f, 340.0f };
	for (const float TemperatureK : TestTemperaturesK)
	{
		GenericThermalSurface->SetTemperatureKelvin(TemperatureK);
		GenericThermalSurface->SetRadianceSensorWorldLocation(CaptureActor->GetSensorWorldLocation());
		ValidationActor->ConfigureSurfaceValidation(GenericThermalSurface, CaptureActor);
		ValidationActor->RunValidation();
		TestTrue(
			*FString::Printf(TEXT("Reusable component CPU/GPU validation completed at %.0f K"), TemperatureK),
			ValidationActor->HasValidationCompleted());
		TestTrue(
			*FString::Printf(TEXT("Reusable component radiance error is within tolerance at %.0f K"), TemperatureK),
			ValidationActor->HasValidationPassed());
	}

	DestroyTestWorld();
	return !HasAnyErrors();
}


#endif
