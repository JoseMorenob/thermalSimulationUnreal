// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "ir_pipeline.h"

#include "IRPipelineConfigAsset.generated.h"

UENUM(BlueprintType)
enum class EIRAgcMode : uint8
{
	LinearMinMax = IR_AGC_LINEAR_MINMAX UMETA(DisplayName = "Linear Min/Max"),
	FixedRange = IR_AGC_FIXED_RANGE UMETA(DisplayName = "Fixed Range (Predefined Min/Max)"),
	PlateauEq = IR_AGC_PLATEAU_EQ UMETA(DisplayName = "Plateau Equalization")
};

UENUM(BlueprintType)
enum class EIRColormap : uint8
{
	WhiteHot = IR_CMAP_WHITE_HOT UMETA(DisplayName = "White Hot"),
	BlackHot = IR_CMAP_BLACK_HOT UMETA(DisplayName = "Black Hot"),
	Ironbow = IR_CMAP_IRONBOW UMETA(DisplayName = "Ironbow"),
	Rainbow = IR_CMAP_RAINBOW UMETA(DisplayName = "Rainbow")
};

// Configuracion editable de IRPipeline. Los valores por defecto se toman de
// ir_default_config; width/height se resuelven en tiempo de ejecucion a
// partir de la resolucion real del render target de radiancia.
UCLASS(BlueprintType)
class IRSIMCLEAN_API  UIRPipelineConfigAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UIRPipelineConfigAsset();

	IRConfig BuildIRConfig(uint32 Width, uint32 Height) const;

	// Stage 1: PSF, optics blur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Optics")
	float PsfSigma;

	// Stage 2: Radiometric calibration  DN = a0 + a1*L + a2*L^2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Radiometric Calibration")
	float CalA0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Radiometric Calibration")
	float CalA1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Radiometric Calibration")
	float CalA2;

	// Stage 3: Detector thermal lag (1st-order IIR)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Thermal Lag")
	float TauFrames;

	// Stage 4: Residual FPN maps
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Fixed Pattern Noise")
	float FpnGainSigma;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Fixed Pattern Noise")
	float FpnOffsetSigma;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Fixed Pattern Noise")
	int32 FpnSeed;

	// Stage 5: Read noise
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Read Noise")
	float NetdDn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Read Noise")
	int32 NoiseSeed;

	// Stage 6: Clamp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Clamp")
	float DnMax;

	// Stage 7: AGC
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|AGC")
	EIRAgcMode AgcMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|AGC")
	float AgcTauFrames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|AGC", meta = (EditCondition = "AgcMode == EIRAgcMode::PlateauEq", EditConditionHides))
	float AgcPlateauPct;

	// Predefined DN range (only used when AgcMode == FixedRange)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|AGC", meta = (EditCondition = "AgcMode == EIRAgcMode::FixedRange", EditConditionHides))
	float AgcFixedMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|AGC", meta = (EditCondition = "AgcMode == EIRAgcMode::FixedRange", EditConditionHides))
	float AgcFixedMax;

	// Stage 8: Colormap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IR Pipeline|Colormap")
	EIRColormap Colormap;
};
