// IRPipelineCore — API C estable para la simulación de detector.
// Autor original: Miguel Gutiérrez, U-tad TFM 2025-2026.

#pragma once

#include <stdint.h>

/* Cámara simulada: Jenoptik IR-TCM HD 1024. Valores de ficha técnica. */

#define IR_TCM_HD_1024_WIDTH         1024u    // columnas nativas del detector
#define IR_TCM_HD_1024_HEIGHT        768u     // filas nativas del detector
#define IR_TCM_HD_1024_RE_WIDTH      2048u    // columnas con Resolution Enhancement
#define IR_TCM_HD_1024_RE_HEIGHT     1536u    // filas con Resolution Enhancement
#define IR_TCM_HD_1024_FPS           30u      // cadencia nativa, Hz
#define IR_TCM_HD_1024_LAMBDA_MIN_UM 7.5f     // límite inferior de banda, µm
#define IR_TCM_HD_1024_LAMBDA_MAX_UM 14.0f    // límite superior de banda, µm
#define IR_TCM_HD_1024_NETD_MK       40.0f    // resolución térmica, mK
#define IR_TCM_HD_1024_DN_BITS       16u      // rango dinámico, bits
#define IR_TCM_HD_1024_DN_MAX        65535.0f // fondo de escala: 2^16 - 1

/* Códigos de resultado de la API. */
typedef enum IRStatus
{
    IR_OK = 0,
    IR_ERR_NULL_INPUT = -1,
    IR_ERR_BAD_CONFIG = -2,
    IR_ERR_NO_OUTPUT  = -3  // Los dos buffers de salida son nulos.
} IRStatus;

/* Modos de control automático de ganancia. */
typedef enum IRAgcMode
{
    IR_AGC_LINEAR_MINMAX = 0,
    IR_AGC_FIXED_RANGE = 1,
    IR_AGC_PLATEAU_EQ = 2
} IRAgcMode;

/* Paletas disponibles para la salida de presentación. */
typedef enum IRColormap
{
    IR_CMAP_WHITE_HOT = 0,
    IR_CMAP_BLACK_HOT = 1,
    IR_CMAP_IRONBOW   = 2,
    IR_CMAP_RAINBOW   = 3
} IRColormap;

/* Parámetros de las etapas que componen el modelo de detector. */
typedef struct IRConfig
{
    // Etapa 0: entrada.
    uint32_t width;  // píxeles.
    uint32_t height; // píxeles.
    uint32_t bands;  // bandas espectrales; 1 significa radiancia integrada.

    // Etapa 1: desenfoque óptico por PSF; cero lo desactiva.
    float psf_sigma;

    // Etapa 2: calibración radiométrica, DN = a0 + a1*L + a2*L².
    float cal_a0;
    float cal_a1;
    float cal_a2;

    // Etapa 3: inercia térmica IIR de primer orden; cero la desactiva.
    float tau_frames;

    // Etapa 4: Fixed Pattern Noise residual.
    float    fpn_gain_sigma;   // desviación de la ganancia por píxel.
    float    fpn_offset_sigma; // desviación del offset por píxel, en DN.
    uint32_t fpn_seed;         // semilla reproducible para el mapa.

    // Etapa 5: ruido de lectura.
    float    netd_dn;    // desviación del ruido, en DN.
    uint32_t noise_seed; // semilla reproducible de ruido.

    // Etapa 6: saturación de la señal digital.
    float dn_max;

    // Etapa 7: control automático de ganancia.
    IRAgcMode agc_mode;
    float     agc_plateau_pct; // porcentaje de plateau para IR_AGC_PLATEAU_EQ.
    float     agc_tau_frames;
    float     agc_fixed_min;   // mínimo DN para IR_AGC_FIXED_RANGE.
    float     agc_fixed_max;   // máximo DN para IR_AGC_FIXED_RANGE.

    // Etapa 8: paleta de visualización.
    IRColormap colormap;
} IRConfig;

/* Manejador opaco: la implementación conserva su estado entre fotogramas. */
typedef struct IRPipeline IRPipeline;

#ifdef __cplusplus
extern "C" {
#endif

// Reserva e inicializa una instancia. Devuelve NULL si la configuración no es válida.
IRPipeline* ir_create(const IRConfig* config);

// Devuelve la configuración por defecto para la resolución de radiancia dada.
IRConfig ir_default_config(uint32_t width, uint32_t height);

// Procesa un fotograma. in_radiance tiene width*height floats. Al menos una
// salida debe ser válida: rgba_out (width*height*4 bytes) o raw_dn_out
// (width*height floats, antes del AGC).
IRStatus ir_process(IRPipeline* pipeline,
                    const float* in_radiance,
                    uint8_t* rgba_out,
                    float* raw_dn_out);

// Libera los recursos de una instancia creada por ir_create.
void ir_destroy(IRPipeline* pipeline);

#ifdef __cplusplus
}
#endif
