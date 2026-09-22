#include "IRSimPlugin.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "IRRuntimeControlActor.h"

#if WITH_EDITOR
#include "DetailLayoutBuilder.h"
#include "IDetailCustomization.h"
#include "IRInternalSceneCaptureComponent.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "RadianceCaptureActor.h"
#endif

#define LOCTEXT_NAMESPACE "FIRSimPluginModule"

#if WITH_EDITOR
namespace
{
class FIRInternalSceneCaptureDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FIRInternalSceneCaptureDetails>();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		// Estas propiedades las fija ARadianceCaptureActor en cada actualizacion.
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent, CaptureSource));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent, bCaptureEveryFrame));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent, bCaptureOnMovement));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent, bAlwaysPersistRenderingState));
		DetailBuilder.HideProperty(TEXT("ShowFlagSettings"), USceneCaptureComponent::StaticClass());
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent2D, TextureTarget));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent2D, PostProcessSettings));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USceneCaptureComponent2D, PostProcessBlendWeight));
	}
};

class FIRRadianceCaptureActorDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FIRRadianceCaptureActorDetails>();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		// La configuracion del componente interno se expone mediante Player View.
		DetailBuilder.HideProperty(TEXT("PlayerViewPostProcessComponent"), ARadianceCaptureActor::StaticClass());
	}
};

}
#endif

void FIRSimPluginModule::StartupModule()
{
	// El panel runtime es una ayuda de demostracion: se crea transitoriamente al iniciar Play.
	// No modifica ni exige colocar actores adicionales en el mapa del usuario.
	WorldBeginPlayHandle = FWorldDelegates::OnWorldInitializedActors.AddRaw(this, &FIRSimPluginModule::CreateRuntimeControlForWorld);
#if WITH_EDITOR
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyEditorModule.RegisterCustomClassLayout(
		UIRInternalSceneCaptureComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FIRInternalSceneCaptureDetails::MakeInstance));
	PropertyEditorModule.RegisterCustomClassLayout(
		ARadianceCaptureActor::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FIRRadianceCaptureActorDetails::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
#endif
}

void FIRSimPluginModule::ShutdownModule()
{
	if (WorldBeginPlayHandle.IsValid())
	{
		FWorldDelegates::OnWorldInitializedActors.Remove(WorldBeginPlayHandle);
		WorldBeginPlayHandle.Reset();
	}
#if WITH_EDITOR
	if (FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
	{
		PropertyEditorModule->UnregisterCustomClassLayout(UIRInternalSceneCaptureComponent::StaticClass()->GetFName());
		PropertyEditorModule->UnregisterCustomClassLayout(ARadianceCaptureActor::StaticClass()->GetFName());
		PropertyEditorModule->NotifyCustomizationModuleChanged();
	}
#endif
}

void FIRSimPluginModule::CreateRuntimeControlForWorld(const FActorsInitializedParams& Params)
{
	UWorld* World = Params.World;
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// Si el nivel ya contiene un controlador configurado manualmente, se respeta.
	for (TActorIterator<AIRRuntimeControlActor> It(World); It; ++It)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transient;
	World->SpawnActor<AIRRuntimeControlActor>(AIRRuntimeControlActor::StaticClass(), FTransform::Identity, SpawnParameters);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FIRSimPluginModule, IRSimPlugin)
