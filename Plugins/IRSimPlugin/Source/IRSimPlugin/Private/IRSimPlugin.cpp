#include "IRSimPlugin.h"

#define LOCTEXT_NAMESPACE "FIRSimPluginModule"

void FIRSimPluginModule::StartupModule() {}
void FIRSimPluginModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FIRSimPluginModule, IRSimPlugin)
