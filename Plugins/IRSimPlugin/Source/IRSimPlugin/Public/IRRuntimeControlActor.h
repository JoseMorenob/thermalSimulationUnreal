#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IRRuntimeControlActor.generated.h"

class AIRSceneEnvironmentActor;
class ARadianceCaptureActor;
class UIRRuntimeControlWidget;
class UIRThermalSurfaceComponent;

// Actor opcional de demostración: crea un panel UMG para modificar la escena IR en Play.
UCLASS(Blueprintable)
class IRSIMPLUGIN_API AIRRuntimeControlActor : public AActor
{
	GENERATED_BODY()

public:
	AIRRuntimeControlActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "IR Runtime Control")
	void ToggleRuntimePanel();

	UFUNCTION(BlueprintCallable, Category = "IR Runtime Control")
	void ShowRuntimePanel();

	UFUNCTION(BlueprintCallable, Category = "IR Runtime Control")
	void HideRuntimePanel();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Runtime Control|Targets")
	TObjectPtr<UIRThermalSurfaceComponent> TargetThermalSurface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Runtime Control|Targets")
	TObjectPtr<AIRSceneEnvironmentActor> SceneEnvironmentActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Runtime Control|Targets")
	TObjectPtr<ARadianceCaptureActor> RadianceCaptureActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Runtime Control")
	// Visible por defecto para que la demostración funcione sin depender de un atajo de teclado.
	bool bShowPanelAtStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Runtime Control")
	int32 ZOrder = 100;

private:
	void CreatePanel();

	UPROPERTY(Transient)
	TObjectPtr<UIRRuntimeControlWidget> RuntimePanel;
};
