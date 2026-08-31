#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Declaracion del modulo que registra el plugin en Unreal

class FIRSimPluginModule : public IModuleInterface {
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
