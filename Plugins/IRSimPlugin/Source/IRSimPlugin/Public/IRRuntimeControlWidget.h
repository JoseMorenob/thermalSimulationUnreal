#pragma once

#include "Blueprint/UserWidget.h"
#include "IRRuntimeControlWidget.generated.h"

class AIRSceneEnvironmentActor;
class ARadianceCaptureActor;
class UButton;
class UCheckBox;
class UIRThermalSurfaceComponent;
class USlider;
class UTextBlock;
class UVerticalBox;

// Panel UMG creado por código para la demostración interactiva del simulador.
UCLASS()
class IRSIMPLUGIN_API UIRRuntimeControlWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureTargets(
		UIRThermalSurfaceComponent* InThermalSurface,
		AIRSceneEnvironmentActor* InEnvironment,
		ARadianceCaptureActor* InCapture);

protected:
	virtual void NativeConstruct() override;

private:
	void ResolveTargets();
	void BuildPanel();
	USlider* AddSliderRow(const FText& Label, TObjectPtr<UTextBlock>& OutValueText);
	UButton* AddButton(const FText& Label);
	void RefreshValues();

	UFUNCTION()
	void OnTemperatureChanged(float Value);
	UFUNCTION()
	void OnAirTemperatureChanged(float Value);
	UFUNCTION()
	void OnExtinctionChanged(float Value);
	UFUNCTION()
	void OnSkyHorizonChanged(float Value);
	UFUNCTION()
	void OnSkyZenithChanged(float Value);
	UFUNCTION()
	void OnBandMinChanged(float Value);
	UFUNCTION()
	void OnBandMaxChanged(float Value);
	UFUNCTION()
	void OnIntegrationSamplesChanged(float Value);
	UFUNCTION()
	void OnDisplayRangeChanged(float Value);
	UFUNCTION()
	void OnCaptureEnabledChanged(bool bEnabled);
	UFUNCTION()
	void OnCaptureNowClicked();
	UFUNCTION()
	void OnAuxiliaryBuffersClicked();
	UFUNCTION()
	void OnRadianceBufferClicked();
	UFUNCTION()
	void OnTemperatureBufferClicked();
	UFUNCTION()
	void OnEmissivityBufferClicked();
	UFUNCTION()
	void OnMaterialIdBufferClicked();

	UPROPERTY(Transient)
	TObjectPtr<UIRThermalSurfaceComponent> ThermalSurface;
	UPROPERTY(Transient)
	TObjectPtr<AIRSceneEnvironmentActor> Environment;
	UPROPERTY(Transient)
	TObjectPtr<ARadianceCaptureActor> Capture;

	UPROPERTY(Transient)
	TObjectPtr<USlider> TemperatureSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> AirTemperatureSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> ExtinctionSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> SkyHorizonSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> SkyZenithSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> BandMinSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> BandMaxSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> IntegrationSamplesSlider;
	UPROPERTY(Transient)
	TObjectPtr<USlider> DisplayRangeSlider;
	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> CaptureEnabledCheckBox;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TemperatureValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AirTemperatureValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExtinctionValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SkyHorizonValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SkyZenithValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BandMinValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BandMaxValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> IntegrationSamplesValue;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DisplayRangeValue;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ControlPanel;
};
