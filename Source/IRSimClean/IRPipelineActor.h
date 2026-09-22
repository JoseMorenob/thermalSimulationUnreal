#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadianceCaptureActor.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "IRPipelineConfigAsset.h"

#include "ir_pipeline.h"

#include "IRPipelineActor.generated.h"

class SWidget;

// Adaptador de demostración entre el buffer de radiancia de Unreal y la
// biblioteca IRPipelineCore. La física de escena permanece en IRSimPlugin;
// este actor solo aplica la cadena de detector y presenta su imagen de salida.
UCLASS()
class IRSIMCLEAN_API AIRPipelineActor : public AActor
{
	GENERATED_BODY()

public:
	// Construye el actor sin reservar recursos de GPU ni de IRPipelineCore.
	AIRPipelineActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Procesa una captura cuando corresponde a la cadencia del detector.
	virtual void Tick(float DeltaTime) override;

	// Reads back the radiance capture's render target and extracts the physical
	// radiance magnitude (R channel) into a flat width*height float buffer
	// suitable for ir_process's in_radiance parameter. Returns false if the
	// capture actor or its render target isn't available yet.
	bool GetRadiancePixels(TArray<float>& OutRadiance) const;

public: // Data members

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline")
	TObjectPtr<ARadianceCaptureActor> RadianceCaptureActor;

	// Configuracion de IRPipeline. Si no se asigna, se usa ir_default_config.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline")
	TObjectPtr<UIRPipelineConfigAsset> PipelineConfigAsset;

	// Salida colorizada de ir_process (rgba_out), actualizada cada Tick.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IR Pipeline", Transient)
	TObjectPtr<UTexture2D> ProcessedTexture;

	// Registra en el log el rango de radiancia (min/max/avg) de cada frame
	// procesado. Utilizado para calibrar Radiometric Calibration; desactivar
	// una vez calibrado, ya que recorre todos los pixeles cada vez que se ejecuta.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Debug")
	bool bLogRadianceStats = false;

	// Registra en el log cuanto tarda cada frame procesado: la lectura del
	// render target de radiancia + la subida del resultado a ProcessedTexture
	// (lado Unreal) frente a la llamada a ir_process (lado IRPipelineCore).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Debug")
	bool bLogPipelineTiming = false;

private: // Data members

	// IRPipelineCore expone un manejador opaco de C; su ciclo de vida se cierra
	// explícitamente con ir_destroy en EndPlay.
	IRPipeline* Pipeline = nullptr;

	uint32 RadianceWidth = 0;
	uint32 RadianceHeight = 0;

	// El pipeline simula un sensor a IR_TCM_HD_1024_FPS; procesar mas a menudo
	// que eso solo repite trabajo de lectura de GPU y de ir_process sin
	// aportar nada, ya que la captura fisica no cambia mas rapido que eso.
	float TimeSinceLastProcess = 0.0f;

	// Buffers reutilizados entre frames para evitar reservar memoria en cada Tick.
	mutable TArray<FLinearColor> CachedLinearPixels;
	TArray<float> CachedRadiancePixels;
	TArray<uint8> CachedRgbaOut;

	FSlateBrush ProcessedTextureBrush;
	TSharedPtr<SWidget> ProcessedTextureWidget;

private: // Methods

	void CreateProcessedTexture(uint32 Width, uint32 Height);
	void UpdateProcessedTexture(const TArray<uint8>& RgbaData);
	void ShowProcessedTextureOnScreen(uint32 Width, uint32 Height);

};
