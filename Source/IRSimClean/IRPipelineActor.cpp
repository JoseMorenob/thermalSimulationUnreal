#include "IRPipelineActor.h"

#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWidget.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

AIRPipelineActor::AIRPipelineActor()
{
	// Tick se limita a la cadencia declarada por el detector en ir_pipeline.h.
	PrimaryActorTick.bCanEverTick = true;
}

void AIRPipelineActor::BeginPlay()
{
	Super::BeginPlay();

	uint32 Width = IR_TCM_HD_1024_WIDTH;
	uint32 Height = IR_TCM_HD_1024_HEIGHT;

	if (RadianceCaptureActor)
	{
		if (UTextureRenderTarget2D* RadianceTarget = RadianceCaptureActor->GetRadianceRenderTarget())
		{
			Width = RadianceTarget->SizeX;
			Height = RadianceTarget->SizeY;
		}
	}

	IRConfig config = PipelineConfigAsset
		? PipelineConfigAsset->BuildIRConfig(Width, Height)
		: ir_default_config(Width, Height);
	Pipeline = ir_create(&config);
	if (!Pipeline)
	{
		UE_LOG(LogTemp, Error, TEXT("No se pudo inicializar IRPipelineCore; se desactiva AIRPipelineActor."));
		SetActorTickEnabled(false);
		return;
	}

	RadianceWidth = Width;
	RadianceHeight = Height;
	CachedRgbaOut.SetNumUninitialized(Width * Height * 4);
	CreateProcessedTexture(Width, Height);
	ShowProcessedTextureOnScreen(Width, Height);
}

void AIRPipelineActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine && GEngine->GameViewport && ProcessedTextureWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ProcessedTextureWidget.ToSharedRef());
	}
	ProcessedTextureWidget.Reset();
	if (Pipeline)
	{
		ir_destroy(Pipeline);
		Pipeline = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AIRPipelineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// El sensor simulado corre a IR_TCM_HD_1024_FPS; reprocesar mas a menudo
	// que eso solo repite la lectura de GPU y ir_process sin ningun cambio
	// util, ya que la captura fisica no se actualiza mas rapido que eso.
	static constexpr float ProcessInterval = 1.0f / IR_TCM_HD_1024_FPS;
	TimeSinceLastProcess += DeltaTime;
	if (TimeSinceLastProcess < ProcessInterval)
	{
		return;
	}
	TimeSinceLastProcess -= ProcessInterval;

	const double ReadbackStart = FPlatformTime::Seconds();
	const float* InRadiance = GetRadiancePixels(CachedRadiancePixels)
		? CachedRadiancePixels.GetData()
		: nullptr;
	const double ReadbackEnd = FPlatformTime::Seconds();

	if (bLogRadianceStats && CachedRadiancePixels.Num() > 0)
	{
		float MinL = TNumericLimits<float>::Max();
		float MaxL = TNumericLimits<float>::Lowest();
		double SumL = 0.0;
		for (float L : CachedRadiancePixels)
		{
			MinL = FMath::Min(MinL, L);
			MaxL = FMath::Max(MaxL, L);
			SumL += L;
		}
		const float AvgL = static_cast<float>(SumL / CachedRadiancePixels.Num());
		UE_LOG(LogTemp, Display, TEXT("Radiance L: min=%f max=%f avg=%f"), MinL, MaxL, AvgL);
	}

	const double IrProcessStart = FPlatformTime::Seconds();
	const IRStatus Result = ir_process(Pipeline, InRadiance, CachedRgbaOut.GetData(), nullptr);
	const double IrProcessEnd = FPlatformTime::Seconds();

	double UploadEnqueueMs = 0.0;
	if (Result == IR_OK)
	{
		const double UploadStart = FPlatformTime::Seconds();
		UpdateProcessedTexture(CachedRgbaOut);
		UploadEnqueueMs = (FPlatformTime::Seconds() - UploadStart) * 1000.0;
	}

	if (bLogPipelineTiming)
	{
		const double ReadbackMs = (ReadbackEnd - ReadbackStart) * 1000.0;
		const double IrProcessMs = (IrProcessEnd - IrProcessStart) * 1000.0;
		// El enqueue de subida de textura solo mide el coste de encolar el
		// comando de render, no la copia real en la GPU (esta se ejecuta mas
		// tarde, de forma asincrona, en el render thread).
		UE_LOG(LogTemp, Display,
			TEXT("IR pipeline timing: UE=%.3fms (readback=%.3fms, upload_enqueue=%.3fms)  ir_process=%.3fms"),
			ReadbackMs + UploadEnqueueMs, ReadbackMs, UploadEnqueueMs, IrProcessMs);
	}
}

void AIRPipelineActor::CreateProcessedTexture(uint32 Width, uint32 Height)
{
	// ir_process escribe RGBA en ese orden; PF_R8G8B8A8 coincide con él.
	// PF_B8G8R8A8 intercambiaría los canales rojo y azul.
	ProcessedTexture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (ProcessedTexture)
	{
		ProcessedTexture->Filter = TF_Nearest;
		ProcessedTexture->SRGB = false;
		ProcessedTexture->UpdateResource();
	}
}

void AIRPipelineActor::UpdateProcessedTexture(const TArray<uint8>& RgbaData)
{
	if (!ProcessedTexture)
	{
		return;
	}

	FTextureResource* Resource = ProcessedTexture->GetResource();
	if (!Resource)
	{
		return;
	}

	const uint32 Width = RadianceWidth;
	const uint32 Height = RadianceHeight;

	ENQUEUE_RENDER_COMMAND(UpdateIRProcessedTexture)(
		[Resource, RgbaData, Width, Height](FRHICommandListImmediate& RHICmdList)
		{
			FRHITexture* TextureRHI = Resource->TextureRHI;
			if (!TextureRHI)
			{
				return;
			}

			uint32 DestStride = 0;
			uint8* DestData = static_cast<uint8*>(
				RHICmdList.LockTexture2D(TextureRHI, 0, RLM_WriteOnly, DestStride, false));

			const uint32 SrcStride = Width * 4;
			for (uint32 Row = 0; Row < Height; ++Row)
			{
				FMemory::Memcpy(DestData + Row * DestStride, RgbaData.GetData() + Row * SrcStride, SrcStride);
			}

			RHICmdList.UnlockTexture2D(TextureRHI, 0, false);
		});
}

void AIRPipelineActor::ShowProcessedTextureOnScreen(uint32 Width, uint32 Height)
{
	if (!ProcessedTexture || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	ProcessedTextureBrush.SetResourceObject(ProcessedTexture);
	ProcessedTextureBrush.ImageSize = FVector2D(Width, Height);

	ProcessedTextureWidget = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Black)
		.Padding(0.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				SNew(SImage)
				.Image(&ProcessedTextureBrush)
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(ProcessedTextureWidget.ToSharedRef());
}

bool AIRPipelineActor::GetRadiancePixels(TArray<float>& OutRadiance) const
{
	if (!RadianceCaptureActor)
	{
		return false;
	}

	UTextureRenderTarget2D* RadianceTarget = RadianceCaptureActor->GetRadianceRenderTarget();
	if (!RadianceTarget)
	{
		return false;
	}

	FTextureRenderTargetResource* Resource = RadianceTarget->GameThread_GetRenderTargetResource();
	if (!Resource)
	{
		return false;
	}

	FReadSurfaceDataFlags ReadFlags(RCM_MinMax);
	ReadFlags.SetLinearToGamma(false);
	if (!Resource->ReadLinearColorPixels(CachedLinearPixels, ReadFlags))
	{
		return false;
	}

	const int32 ExpectedPixelCount = RadianceTarget->SizeX * RadianceTarget->SizeY;
	if (CachedLinearPixels.Num() != ExpectedPixelCount)
	{
		return false;
	}

	OutRadiance.SetNumUninitialized(CachedLinearPixels.Num());
	for (int32 Index = 0; Index < CachedLinearPixels.Num(); ++Index)
	{
		OutRadiance[Index] = CachedLinearPixels[Index].R;
	}

	return true;
}
