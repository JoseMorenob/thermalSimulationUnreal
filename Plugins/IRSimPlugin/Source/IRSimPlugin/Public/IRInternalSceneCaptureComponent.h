#pragma once

#include "Components/SceneCaptureComponent2D.h"

#include "IRInternalSceneCaptureComponent.generated.h"

// Componente de captura propiedad del plugin. Su configuracion de salida se
// gestiona desde ARadianceCaptureActor y no debe editarse directamente.
UCLASS(NotBlueprintable)
class IRSIMPLUGIN_API UIRInternalSceneCaptureComponent final : public USceneCaptureComponent2D
{
	GENERATED_BODY()
};
