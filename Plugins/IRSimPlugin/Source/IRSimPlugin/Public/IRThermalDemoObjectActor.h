#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IRThermalDemoObjectActor.generated.h"

class UIRThermalSurfaceComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class IRSIMPLUGIN_API AIRThermalDemoObjectActor : public AActor
{
	GENERATED_BODY()

public:
	AIRThermalDemoObjectActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Demo")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Demo")
	TObjectPtr<UIRThermalSurfaceComponent> ThermalSurface;
};
