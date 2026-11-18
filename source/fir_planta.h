/*
 * fir_planta.h - Planta a identificar: filtro FIR de 30 coeficientes en Q15 (CMSIS-DSP).
 */
#ifndef FIR_PLANTA_H
#define FIR_PLANTA_H

#include <stdint.h>
#include "arm_math.h"               /* CMSIS-DSP */

#define PLANTA_NTAPS 30
#define BLOCK_SIZE	1

/* Coeficientes arbitrarios y fijos, Q15, orden natural. */
static const q15_t PLANTA_COEF[PLANTA_NTAPS] = {
     4224,  3007,   570, -1902, -2985, -2525, -1536,  -802,  -169,   717,
     1549,  1658,   945,    55,  -399,  -466,  -536,  -681,  -602,  -162,
      334,   503,   346,   167,   133,   111,   -53,  -263,  -306,  -146
};

static q15_t planta_state[PLANTA_NTAPS + BLOCK_SIZE];
static arm_fir_instance_q15 planta_inst;

#endif /* FIR_PLANTA_H */
