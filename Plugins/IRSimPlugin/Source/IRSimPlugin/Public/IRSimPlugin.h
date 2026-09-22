#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct FActorsInitializedParams;

// Declaracion del modulo que registra el plugin en Unreal

class FIRSimPluginModule : public IModuleInterface {
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void CreateRuntimeControlForWorld(const FActorsInitializedParams& Params);

    FDelegateHandle WorldBeginPlayHandle;
};
