/*
 * fir_adaptativo.h - Estructuras y Datos nesesarios para el filtro.
 */
#ifndef FIR_ADAPTATIVO_H_
#define FIR_ADAPTATIVO_H_

#include <stdint.h>
#include "arm_math.h"               /* CMSIS-DSP */

/** Cantidad de valores posibles para MU disponibles. */
#define CANT_MU_VALUES    	      6U

/** Cantidad de coeficientes del filtro adaptativo */
#define NTAPS_ADAPTATIVO		  30

/* ------------------------------------------------------------------------- */
/* Parte 1: Identificacion de planta (LMS en Q15)                            */
/* ------------------------------------------------------------------------- */

/* mu en Q15: 0.05  0.10  0.20  0.35  0.50  0.70 */
static const int16_t MU_VALUES[CANT_MU_VALUES] = { 1638, 3277, 6554, 11469, 16384, 22938 };

/** Coeficientes Adaptativos */
static q15_t w[NTAPS_ADAPTATIVO];

/** Estado del filtro (numTaps + blockSize - 1) */
static q15_t lms_state[NTAPS_ADAPTATIVO];

/** Instancia del filtro */
static arm_lms_instance_q15 lms;

/* ------------------------------------------------------------------------- */
/* Parte 2: Cancelacion de ruido (LMS en punto flotante)                     */
/* ------------------------------------------------------------------------- */

/* mu en float para la cancelacion de ruido */
static const float32_t MU_CANC[CANT_MU_VALUES] = { 0.001f, 0.002f, 0.005f, 0.01f, 0.02f, 0.05f };

/** Coeficientes adaptativos (float) */
static float32_t w_f[NTAPS_ADAPTATIVO];

/** Estado del filtro (numTaps + blockSize - 1) */
static float32_t lms_state_f[NTAPS_ADAPTATIVO];

/** Instancia del filtro (float) */
static arm_lms_instance_f32 lms_f;

/* ------------------------------------------------------------------------- */
/* Variables comunes                                                         */
/* ------------------------------------------------------------------------- */

/* Indice del valor de MU actual*/
static uint8_t i_mu = 0;

/** Numero de experimento */
static uint32_t seq = 0;

/* Acumulador para el calculo del MSE */
static int64_t acc = 0;

/* Cantidad de Muestras de la ventana actual*/
static uint32_t n = 0;

/** Valor del MSE */
static int32_t mse;

/* Arreglo con la diferencia entre los coeficientes definidos y los estimados: d[i] = coef[i] - w[i]*/
static int16_t dw[NTAPS_ADAPTATIVO];

#endif /* FIR_ADAPTATIVO_H_ */
