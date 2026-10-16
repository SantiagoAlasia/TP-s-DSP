/**
 * @file    fir_filter.h
 * @brief   Tabla de filtros FIR (coeficientes en Q15) por tipo de filtro y por
 *          frecuencia de muestreo.
 *
 * Los coeficientes dependen de la frecuencia de muestreo, por eso hay un juego
 * por cada combinacion (filtro, fs)
 */
#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <stdint.h>
#include "arm_math.h"      /* q15_t */

/* ========================================================================== */
/*  PASA BAJOS  (Fc = 3600 Hz, Astop = 30 dB)                                 */
/* ========================================================================== */
/* fs = 8 kS/s: 68 taps */

#define BLOCK_SIZE 1
#define NUM_TAPS_PB_8K 64

static const q15_t pb_8k[] = {
		119, -146, 161, -159, 139, -98, 37, 40,
		-127, 215, -295, 356, -387, 380, -329, 230,
		-87, -95, 303, -521, 727, -898, 1008, -1031,
		939, -703, 289, 356, -1336, 2927, -6166, 20535,
		20535, -6166, 2927, -1336, 356, 289, -703, 939,
		-1031, 1008, -898, 727, -521, 303, -95, -87,
		230, -329, 380, -387, 356, -295, 215, -127,
		40, 37, -98, 139, -159, 161, -146, 119
};

#endif /* FIR_FILTER_H */
