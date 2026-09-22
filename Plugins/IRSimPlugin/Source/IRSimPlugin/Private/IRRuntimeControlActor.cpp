#include "IRRuntimeControlActor.h"

#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "IRRuntimeControlWidget.h"
#include "IRSceneEnvironmentActor.h"
#include "IRThermalSurfaceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RadianceCaptureActor.h"
#include "TimerManager.h"

AIRRuntimeControlActor::AIRRuntimeControlActor()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoReceiveInput = EAutoReceiveInput::Player0;
}

void AIRRuntimeControlActor::BeginPlay()
{
	Super::BeginPlay();
	CreatePanel();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		EnableInput(PlayerController);
		if (InputComponent)
		{
			FInputKeyBinding& ToggleBinding = InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AIRRuntimeControlActor::ToggleRuntimePanel);
			ToggleBinding.bConsumeInput = false;
		}
	}

	if (bShowPanelAtStart)
	{
		ShowRuntimePanel();
	}
}

void AIRRuntimeControlActor::CreatePanel()
{
	if (RuntimePanel)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		// El controlador puede aparecer un frame despues del comienzo del mundo en PIE.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(this, &AIRRuntimeControlActor::CreatePanel);
		}
		return;
	}

	RuntimePanel = CreateWidget<UIRRuntimeControlWidget>(PlayerController, UIRRuntimeControlWidget::StaticClass());
	if (RuntimePanel)
	{
		RuntimePanel->ConfigureTargets(TargetThermalSurface, SceneEnvironmentActor, RadianceCaptureActor);
	}
}

void AIRRuntimeControlActor::ToggleRuntimePanel()
{
	CreatePanel();
	if (!RuntimePanel)
	{
		return;
	}

	if (RuntimePanel->IsInViewport())
	{
		HideRuntimePanel();
	}
	else
	{
		ShowRuntimePanel();
	}
}

void AIRRuntimeControlActor::ShowRuntimePanel()
{
	CreatePanel();
	if (!RuntimePanel || RuntimePanel->IsInViewport())
	{
		return;
	}

	RuntimePanel->AddToViewport(ZOrder);
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(RuntimePanel->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}

void AIRRuntimeControlActor::HideRuntimePanel()
{
	if (!RuntimePanel)
	{
		return;
	}

	RuntimePanel->RemoveFromParent();
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}
}
