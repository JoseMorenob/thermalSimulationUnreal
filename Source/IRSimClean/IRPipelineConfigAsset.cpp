// Fill out your copyright notice in the Description page of Project Settings.

#include "IRPipelineConfigAsset.h"

UIRPipelineConfigAsset::UIRPipelineConfigAsset()
{
	// width/height no importan para el resto de los valores por defecto;
	// se recalculan en BuildIRConfig con la resolucion real de captura.
	const IRConfig Defaults = ir_default_config(1, 1);

	PsfSigma = Defaults.psf_sigma;
	CalA0 = Defaults.cal_a0;
	CalA1 = Defaults.cal_a1;
	CalA2 = Defaults.cal_a2;
	TauFrames = Defaults.tau_frames;
	FpnGainSigma = Defaults.fpn_gain_sigma;
	FpnOffsetSigma = Defaults.fpn_offset_sigma;
	FpnSeed = static_cast<int32>(Defaults.fpn_seed);
	NetdDn = Defaults.netd_dn;
	NoiseSeed = static_cast<int32>(Defaults.noise_seed);
	DnMax = Defaults.dn_max;
	AgcMode = static_cast<EIRAgcMode>(Defaults.agc_mode);
	AgcPlateauPct = Defaults.agc_plateau_pct;
	AgcTauFrames = Defaults.agc_tau_frames;
	AgcFixedMin = Defaults.agc_fixed_min;
	AgcFixedMax = Defaults.agc_fixed_max;
	Colormap = static_cast<EIRColormap>(Defaults.colormap);
}

IRConfig UIRPipelineConfigAsset::BuildIRConfig(uint32 Width, uint32 Height) const
{
	IRConfig Config = ir_default_config(Width, Height);

	Config.psf_sigma = PsfSigma;
	Config.cal_a0 = CalA0;
	Config.cal_a1 = CalA1;
	Config.cal_a2 = CalA2;
	Config.tau_frames = TauFrames;
	Config.fpn_gain_sigma = FpnGainSigma;
	Config.fpn_offset_sigma = FpnOffsetSigma;
	Config.fpn_seed = static_cast<uint32_t>(FpnSeed);
	Config.netd_dn = NetdDn;
	Config.noise_seed = static_cast<uint32_t>(NoiseSeed);
	Config.dn_max = DnMax;
	Config.agc_mode = static_cast<IRAgcMode>(AgcMode);
	Config.agc_plateau_pct = AgcPlateauPct;
	Config.agc_tau_frames = AgcTauFrames;
	Config.agc_fixed_min = AgcFixedMin;
	Config.agc_fixed_max = AgcFixedMax;
	Config.colormap = static_cast<IRColormap>(Colormap);

	return Config;
}
