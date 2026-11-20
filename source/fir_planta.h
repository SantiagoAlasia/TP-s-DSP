/*
 * fir_planta.h - Planta a identificar: filtro FIR de 30 coeficientes en Q15 (CMSIS-DSP).
 */
#ifndef FIR_PLANTA_H
#define FIR_PLANTA_H

#include <stdint.h>
#include "arm_math.h"               /* CMSIS-DSP */

#define PLANTA_NTAPS 30
#define BLOCK_SIZE	1

/** Coeficientes arbitrarios y fijos, Q15, en orden invertido (formato CMSIS)*/
static const q15_t PLANTA_COEF[PLANTA_NTAPS] = {
     -390,  -436,  -340,   -96,   253,
      614,   863,   879,   589,     6,
     -749, -1459, -1860, -1719,  -929,
      420,  2013,  3358,  3907,  3234,
     1220, -1811, -5085, -7514, -7992,
    -5782,  -883,  5755, 12292, 16384
};

static q15_t planta_state[PLANTA_NTAPS + BLOCK_SIZE];
static arm_fir_instance_q15 planta_inst;

#endif /* FIR_PLANTA_H */
