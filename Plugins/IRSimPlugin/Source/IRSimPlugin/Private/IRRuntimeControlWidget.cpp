#include "IRRuntimeControlWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "IRSceneEnvironmentActor.h"
#include "IRThermalSurfaceComponent.h"
#include "RadianceCaptureActor.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	constexpr float MinSurfaceTemperatureK = 200.0f;
	constexpr float MaxSurfaceTemperatureK = 800.0f;
	constexpr float MinAirTemperatureK = 180.0f;
	constexpr float MaxAirTemperatureK = 360.0f;
	constexpr float MaxExtinction = 0.10f;
	constexpr float MinSkyTemperatureK = 150.0f;
	constexpr float MaxSkyTemperatureK = 330.0f;
	constexpr float MinBandMicrons = 3.0f;
	constexpr float MaxBandMicrons = 20.0f;
	constexpr int32 MinIntegrationSamples = 4;
	constexpr int32 MaxIntegrationSamples = 256;
	constexpr float MinDisplayRadiance = 1.0f;
	constexpr float MaxDisplayRadiance = 250.0f;
}

void UIRRuntimeControlWidget::ConfigureTargets(
	UIRThermalSurfaceComponent* InThermalSurface,
	AIRSceneEnvironmentActor* InEnvironment,
	ARadianceCaptureActor* InCapture)
{
	ThermalSurface = InThermalSurface;
	Environment = InEnvironment;
	Capture = InCapture;
}

void UIRRuntimeControlWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveTargets();
	BuildPanel();
	RefreshValues();
}

void UIRRuntimeControlWidget::ResolveTargets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!Environment)
	{
		for (TActorIterator<AIRSceneEnvironmentActor> It(World); It; ++It)
		{
			Environment = *It;
			break;
		}
	}
	if (!Capture)
	{
		for (TActorIterator<ARadianceCaptureActor> It(World); It; ++It)
		{
			Capture = *It;
			break;
		}
	}
	if (!ThermalSurface)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UIRThermalSurfaceComponent* Candidate = It->FindComponentByClass<UIRThermalSurfaceComponent>())
			{
				ThermalSurface = Candidate;
				break;
			}
		}
	}
}

void UIRRuntimeControlWidget::BuildPanel()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetPadding(FMargin(14.0f, 12.0f));
	Background->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.10f, 0.96f));
	UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background);
	BackgroundSlot->SetPosition(FVector2D(24.0f, 24.0f));
	BackgroundSlot->SetSize(FVector2D(430.0f, 385.0f));

	ControlPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Background->SetContent(ControlPanel);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("IR SIM CONTROL DE ESCENA")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.40f, 0.82f, 0.94f)));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	ControlPanel->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));

	TemperatureSlider = AddSliderRow(FText::FromString(TEXT("Temperatura superficie")), TemperatureValue);
	TemperatureSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnTemperatureChanged);
	AirTemperatureSlider = AddSliderRow(FText::FromString(TEXT("Temperatura aire")), AirTemperatureValue);
	AirTemperatureSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnAirTemperatureChanged);
	ExtinctionSlider = AddSliderRow(FText::FromString(TEXT("Extinción atmósfera")), ExtinctionValue);
	ExtinctionSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnExtinctionChanged);
	SkyHorizonSlider = AddSliderRow(FText::FromString(TEXT("Temperatura horizonte")), SkyHorizonValue);
	SkyHorizonSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnSkyHorizonChanged);
	SkyZenithSlider = AddSliderRow(FText::FromString(TEXT("Temperatura cenit")), SkyZenithValue);
	SkyZenithSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnSkyZenithChanged);
	BandMinSlider = AddSliderRow(FText::FromString(TEXT("Rango inicio")), BandMinValue);
	BandMinSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnBandMinChanged);
	BandMaxSlider = AddSliderRow(FText::FromString(TEXT("Rango final")), BandMaxValue);
	BandMaxSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnBandMaxChanged);
	IntegrationSamplesSlider = AddSliderRow(FText::FromString(TEXT("Muestras integración")), IntegrationSamplesValue);
	IntegrationSamplesSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnIntegrationSamplesChanged);
	DisplayRangeSlider = AddSliderRow(FText::FromString(TEXT("Máximo visualización")), DisplayRangeValue);
	DisplayRangeSlider->OnValueChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnDisplayRangeChanged);

	CaptureEnabledCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	CaptureEnabledCheckBox->SetContent(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()));
	UTextBlock* CaptureLabel = Cast<UTextBlock>(CaptureEnabledCheckBox->GetContent());
	CaptureLabel->SetText(FText::FromString(TEXT("Captura continua")));
	CaptureLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	CaptureLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.87f, 0.90f)));
	CaptureEnabledCheckBox->OnCheckStateChanged.AddDynamic(this, &UIRRuntimeControlWidget::OnCaptureEnabledChanged);
	ControlPanel->AddChildToVerticalBox(CaptureEnabledCheckBox)->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 5.0f));

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ControlPanel->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
	UButton* CaptureNow = AddButton(FText::FromString(TEXT("Capturar")));
	CaptureNow->OnClicked.AddDynamic(this, &UIRRuntimeControlWidget::OnCaptureNowClicked);
	Actions->AddChildToHorizontalBox(CaptureNow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* Auxiliary = AddButton(FText::FromString(TEXT("Buffers aux.")));
	Auxiliary->OnClicked.AddDynamic(this, &UIRRuntimeControlWidget::OnAuxiliaryBuffersClicked);
	Actions->AddChildToHorizontalBox(Auxiliary)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* Buffers = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Buffers->SetText(FText::GetEmpty());
	Buffers->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBox* BufferButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ControlPanel->AddChildToVerticalBox(BufferButtons)->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
	UButton* Radiance = AddButton(FText::FromString(TEXT("Radiancia")));
	Radiance->OnClicked.AddDynamic(this, &UIRRuntimeControlWidget::OnRadianceBufferClicked);
	BufferButtons->AddChildToHorizontalBox(Radiance)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* Temperature = AddButton(FText::FromString(TEXT("Temp.")));
	Temperature->OnClicked.AddDynamic(this, &UIRRuntimeControlWidget::OnTemperatureBufferClicked);
	BufferButtons->AddChildToHorizontalBox(Temperature)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* Emissivity = AddButton(FText::FromString(TEXT("Emis.")));
	Emissivity->OnClicked.AddDynamic(this, &UIRRuntimeControlWidget::OnEmissivityBufferClicked);
	BufferButtons->AddChildToHorizontalBox(Emissivity)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* MaterialId = AddButton(FText::FromString(TEXT("ID")));
	MaterialId->OnClicked.AddDynamic(this, &UIRRuntimeControlWidget::OnMaterialIdBufferClicked);
	BufferButtons->AddChildToHorizontalBox(MaterialId)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}

USlider* UIRRuntimeControlWidget::AddSliderRow(const FText& Label, TObjectPtr<UTextBlock>& OutValueText)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ControlPanel->AddChildToVerticalBox(Row);
	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.87f, 0.90f)));
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	LabelSlot->SetPadding(FMargin(0.0f, 1.0f, 8.0f, 1.0f));
	LabelSlot->SetHorizontalAlignment(HAlign_Left);
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider);
	SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SliderSlot->SetPadding(FMargin(0.0f, 4.0f, 8.0f, 2.0f));
	OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutValueText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	OutValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.97f)));
	UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValueText);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ValueSlot->SetHorizontalAlignment(HAlign_Right);
	return Slider;
}

UButton* UIRRuntimeControlWidget::AddButton(const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.91f, 0.93f)));
	Button->SetContent(Text);
	Button->SetBackgroundColor(FLinearColor(0.13f, 0.20f, 0.26f, 1.0f));
	Button->SetColorAndOpacity(FLinearColor::White);
	return Button;
}

void UIRRuntimeControlWidget::RefreshValues()
{
	// Los parámetros pueden venir de actores configurados fuera del panel.
	// Se limitan al intervalo visible para que el control siga siendo válido.
	if (ThermalSurface && TemperatureSlider)
	{
		const float Temperature = ThermalSurface->GetTemperatureKelvin();
		TemperatureSlider->SetValue(FMath::Clamp(
			(Temperature - MinSurfaceTemperatureK) / (MaxSurfaceTemperatureK - MinSurfaceTemperatureK), 0.0f, 1.0f));
		TemperatureValue->SetText(FText::AsNumber(FMath::RoundToInt(Temperature)));
	}
	if (Environment && AirTemperatureSlider)
	{
		const float Air = Environment->GetAirTemperatureK();
		AirTemperatureSlider->SetValue(FMath::Clamp(
			(Air - MinAirTemperatureK) / (MaxAirTemperatureK - MinAirTemperatureK), 0.0f, 1.0f));
		AirTemperatureValue->SetText(FText::AsNumber(FMath::RoundToInt(Air)));
		const float Extinction = Environment->GetAtmosphericExtinctionCoefficient();
		ExtinctionSlider->SetValue(FMath::Clamp(Extinction / MaxExtinction, 0.0f, 1.0f));
		ExtinctionValue->SetText(FText::AsNumber(Extinction));
		const float Horizon = Environment->GetSkyHorizonTemperatureK();
		SkyHorizonSlider->SetValue(FMath::Clamp(
			(Horizon - MinSkyTemperatureK) / (MaxSkyTemperatureK - MinSkyTemperatureK), 0.0f, 1.0f));
		SkyHorizonValue->SetText(FText::AsNumber(FMath::RoundToInt(Horizon)));
		const float Zenith = Environment->GetSkyZenithTemperatureK();
		SkyZenithSlider->SetValue(FMath::Clamp(
			(Zenith - MinSkyTemperatureK) / (MaxSkyTemperatureK - MinSkyTemperatureK), 0.0f, 1.0f));
		SkyZenithValue->SetText(FText::AsNumber(FMath::RoundToInt(Zenith)));
		const float BandMin = Environment->GetBandMinMicrons();
		BandMinSlider->SetValue(FMath::Clamp(
			(BandMin - MinBandMicrons) / (MaxBandMicrons - MinBandMicrons), 0.0f, 1.0f));
		BandMinValue->SetText(FText::AsNumber(BandMin));
		const float BandMax = Environment->GetBandMaxMicrons();
		BandMaxSlider->SetValue(FMath::Clamp(
			(BandMax - MinBandMicrons) / (MaxBandMicrons - MinBandMicrons), 0.0f, 1.0f));
		BandMaxValue->SetText(FText::AsNumber(BandMax));
		const int32 Samples = Environment->GetSpectralIntegrationSamples();
		IntegrationSamplesSlider->SetValue(FMath::Clamp(
			static_cast<float>(Samples - MinIntegrationSamples) /
			static_cast<float>(MaxIntegrationSamples - MinIntegrationSamples), 0.0f, 1.0f));
		IntegrationSamplesValue->SetText(FText::AsNumber(Samples));
	}
	if (Capture && DisplayRangeSlider)
	{
		const float DisplayMax = Capture->GetDisplayRadianceMax();
		DisplayRangeSlider->SetValue(FMath::Clamp(
			(DisplayMax - MinDisplayRadiance) / (MaxDisplayRadiance - MinDisplayRadiance), 0.0f, 1.0f));
		DisplayRangeValue->SetText(FText::AsNumber(FMath::RoundToInt(DisplayMax)));
		CaptureEnabledCheckBox->SetIsChecked(Capture->IsRadianceCaptureEnabled());
	}
}

void UIRRuntimeControlWidget::OnTemperatureChanged(float Value)
{
	const float Temperature = FMath::Lerp(MinSurfaceTemperatureK, MaxSurfaceTemperatureK, Value);
	if (ThermalSurface)
	{
		ThermalSurface->SetTemperatureKelvin(Temperature);
	}
	if (TemperatureValue)
	{
		TemperatureValue->SetText(FText::AsNumber(FMath::RoundToInt(Temperature)));
	}
}
void UIRRuntimeControlWidget::OnAirTemperatureChanged(float Value)
{
	const float Temperature = FMath::Lerp(MinAirTemperatureK, MaxAirTemperatureK, Value);
	if (Environment)
	{
		Environment->SetAirTemperatureKelvin(Temperature);
	}
	if (AirTemperatureValue)
	{
		AirTemperatureValue->SetText(FText::AsNumber(FMath::RoundToInt(Temperature)));
	}
}
void UIRRuntimeControlWidget::OnExtinctionChanged(float Value)
{
	const float Extinction = Value * MaxExtinction;
	if (Environment)
	{
		Environment->SetAtmosphericExtinctionCoefficient(Extinction);
	}
	if (ExtinctionValue)
	{
		ExtinctionValue->SetText(FText::AsNumber(Extinction));
	}
}
void UIRRuntimeControlWidget::OnSkyHorizonChanged(float Value)
{
	const float Temperature = FMath::Lerp(MinSkyTemperatureK, MaxSkyTemperatureK, Value);
	if (Environment)
	{
		Environment->SetSkyTemperaturesKelvin(Temperature, Environment->GetSkyZenithTemperatureK());
	}
	if (SkyHorizonValue)
	{
		SkyHorizonValue->SetText(FText::AsNumber(FMath::RoundToInt(Temperature)));
	}
}
void UIRRuntimeControlWidget::OnSkyZenithChanged(float Value)
{
	const float Temperature = FMath::Lerp(MinSkyTemperatureK, MaxSkyTemperatureK, Value);
	if (Environment)
	{
		Environment->SetSkyTemperaturesKelvin(Environment->GetSkyHorizonTemperatureK(), Temperature);
	}
	if (SkyZenithValue)
	{
		SkyZenithValue->SetText(FText::AsNumber(FMath::RoundToInt(Temperature)));
	}
}
void UIRRuntimeControlWidget::OnBandMinChanged(float Value)
{
	const float BandMin = FMath::Lerp(MinBandMicrons, MaxBandMicrons, Value);
	if (Environment)
	{
		Environment->SetSpectralBand(BandMin, Environment->GetBandMaxMicrons(), Environment->GetSpectralIntegrationSamples());
	}
	if (BandMinValue)
	{
		BandMinValue->SetText(FText::AsNumber(BandMin));
	}
}
void UIRRuntimeControlWidget::OnBandMaxChanged(float Value)
{
	const float BandMax = FMath::Lerp(MinBandMicrons, MaxBandMicrons, Value);
	if (Environment)
	{
		Environment->SetSpectralBand(Environment->GetBandMinMicrons(), BandMax, Environment->GetSpectralIntegrationSamples());
	}
	if (BandMaxValue)
	{
		BandMaxValue->SetText(FText::AsNumber(BandMax));
	}
}
void UIRRuntimeControlWidget::OnIntegrationSamplesChanged(float Value)
{
	const int32 Samples = FMath::RoundToInt(FMath::Lerp(static_cast<float>(MinIntegrationSamples), static_cast<float>(MaxIntegrationSamples), Value));
	if (Environment)
	{
		Environment->SetSpectralBand(Environment->GetBandMinMicrons(), Environment->GetBandMaxMicrons(), Samples);
	}
	if (IntegrationSamplesValue)
	{
		IntegrationSamplesValue->SetText(FText::AsNumber(Samples));
	}
}
void UIRRuntimeControlWidget::OnDisplayRangeChanged(float Value)
{
	const float DisplayMax = FMath::Lerp(MinDisplayRadiance, MaxDisplayRadiance, Value);
	if (Capture)
	{
		Capture->SetDisplayRadianceRange(0.0f, DisplayMax);
	}
	if (DisplayRangeValue)
	{
		DisplayRangeValue->SetText(FText::AsNumber(FMath::RoundToInt(DisplayMax)));
	}
}

void UIRRuntimeControlWidget::OnCaptureEnabledChanged(bool bEnabled)
{
	if (Capture)
	{
		Capture->SetRadianceCaptureEnabled(bEnabled);
	}
}

void UIRRuntimeControlWidget::OnCaptureNowClicked()
{
	if (Capture)
	{
		Capture->CaptureRadianceFrame();
	}
}

void UIRRuntimeControlWidget::OnAuxiliaryBuffersClicked()
{
	if (Capture)
	{
		Capture->SetCaptureAuxiliaryBuffers(true);
	}
}

void UIRRuntimeControlWidget::OnRadianceBufferClicked()
{
	if (Capture)
	{
		Capture->SetDebugBuffer(EIRDebugBuffer::Radiance);
	}
}

void UIRRuntimeControlWidget::OnTemperatureBufferClicked()
{
	if (Capture)
	{
		Capture->SetDebugBuffer(EIRDebugBuffer::Temperature);
	}
}

void UIRRuntimeControlWidget::OnEmissivityBufferClicked()
{
	if (Capture)
	{
		Capture->SetDebugBuffer(EIRDebugBuffer::Emissivity);
	}
}

void UIRRuntimeControlWidget::OnMaterialIdBufferClicked()
{
	if (Capture)
	{
		Capture->SetDebugBuffer(EIRDebugBuffer::MaterialId);
	}
}
