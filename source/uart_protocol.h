/*
 * uart_protocol.h - Mensajes UART del TP4 (texto, una línea por mensaje, termina en \r\n).
 *
 * Mensajes Posibles:
 * 	1. C,seq,modo,mu_idx,mu_q15                  configuración (al cambiar etapa o mu)
 *  2. D,seq,blk,drop,mse_q30,dw0,...,dw29       datos (una línea por bloque de 100 muestras)
 *
 * Valores crudos en Q15/Q30 (no se usan floats). Se convierten en Octave.
 */

#ifndef UART_PROTOCOL_H_
#define UART_PROTOCOL_H_

#include <stdint.h>
#include "fsl_debug_console.h"

#define TX_NTAPS 30

/* modo: 0 = OFF, 1 = IDEN_RAND, 2 = IDEN_CONST, 3 = CANC */
static inline void tx_cfg(uint32_t seq, uint8_t modo, uint8_t mu_idx, int16_t mu_q15)
{
    PRINTF("C,%u,%u,%u,%d\r\n", (unsigned)seq, (unsigned)modo, (unsigned)mu_idx, (int)mu_q15);
}

/*
 * mse_q30: MSE crudo (real = mse_q30 / 2^30).
 * dw[k]: planta[k] - w[k], crudo en Q15 (real = dw / 32768).
 */
static inline void tx_data(uint32_t seq, int32_t mse_q30, const int16_t dw[TX_NTAPS])
{
    PRINTF("D,%u,%d", (unsigned)seq, (int)mse_q30);
    for (int k = 0; k < TX_NTAPS; k++) {
        PRINTF(",%d", (int)dw[k]);
    }
    PRINTF("\r\n");
}

#endif /* UART_PROTOCOL_H_ */
