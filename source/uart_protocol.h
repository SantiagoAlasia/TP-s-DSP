/*
 * uart_protocol.h - Funciones para el envio de los datos segun el formato establecido en el script de octave.
 */
#ifndef UART_PROTOCOL_H_
#define UART_PROTOCOL_H_

#include <stdio.h>
#include <stdint.h>
#include "fsl_debug_console.h"

#define TX_NTAPS 30

static inline void tx_cfg(uint32_t seq, uint8_t modo, uint8_t mu_idx, int16_t mu_q15)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "C,%u,%u,%u,%d\r\n",
             (unsigned)seq, (unsigned)modo, (unsigned)mu_idx, (int)mu_q15);
    PRINTF("%s", buf);
}

static inline void tx_data(uint32_t seq, int32_t mse_q30, const int16_t dw[TX_NTAPS])
{
    char buf[300];
    int pos = snprintf(buf, sizeof(buf), "D,%u,%ld", (unsigned)seq, (long)mse_q30);
    for (int k = 0; k < TX_NTAPS; k++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, ",%d", (int)dw[k]);
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\r\n");
    PRINTF("%s", buf);
}

#endif /* UART_PROTOCOL_H_ */
