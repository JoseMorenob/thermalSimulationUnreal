#include "IRThermalDemoObjectActor.h"

#include "Components/StaticMeshComponent.h"
#include "IRThermalSurfaceComponent.h"

AIRThermalDemoObjectActor::AIRThermalDemoObjectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	ThermalSurface = CreateDefaultSubobject<UIRThermalSurfaceComponent>(TEXT("ThermalSurface"));
}
