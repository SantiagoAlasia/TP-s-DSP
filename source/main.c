/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    main.c
 * @brief   Laboratorio 4 - Sistemas Adaptativos con MCXN947.
 *
 * Implementacion de dos sistemas adaptativos para diferentes usos:
 * 1. Identificador de Planta
 * 2. Cancelacion de Ruido
 *
 * Para permitir esto se implementara una FSM, con 4 estados:
 * 1. OFF
 * 2. Identificador con valores de entrada random
 * 3. Identificador con valor de entrada constante
 * 4. Cancelacion de Ruido
 *
 * Mediante el SW2 vamos a poder cambiar de modo de uso, y con el SW3 se aumentara el parametro "mu".
 *
 * Para el estado 4, ademas se realizara una adquisicion a 8kS/s.
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "arm_math.h"
#include "fsl_debug_console.h"
#include "fsl_lpadc.h"
#include "fsl_dac.h"

#include <fir_planta.h>
#include <fir_adaptativo.h>
#include <uart_protocol.h>

/* Modo Debug */
//#define MODO_DEBUG                1   // 1 = imprime por PRINTF, 0 = no imprime nada

/** Pin del boton de cambio de modo (SW3) dentro del puerto GPIO0. */
#define PIN_START_STOP            6U

/** Pin del boton de cambio de valor de mu (SW3) dentro del puerto GPIO0. */
#define PIN_SWITCH_FREC           23U

/** Cantidad de estados de frecuencia disponibles. */
#define CANT_ESTADOS       	      4U

/**
 * Estados de la maquina de estados principal.
 */
typedef enum
{
    ESTADO_OFF   = 0,  		/* ADC OFF, DAC OFF, LEDs OFF */
	ESTADO_IDEN_RAND = 1,	/* ADC OFF, DAC OFF, LEDs = ROJO */
	ESTADO_IDEN_CONST = 2,  /* ADC OFF, DAC OFF, LEDs = VERDE */
	ESTADO_CANCELACION = 3  /* ADC ON, DAC ON, LEDs = AZUL */
} estado_muestreo_t;

/** Estado actual de la maquina de estados. */
estado_muestreo_t f_estado = ESTADO_OFF;

/** Valores intermedios del diseño */
q15_t x, d, y, e;

/** Envio de datos pendiente */
volatile uint8_t flag_enviar = 0;

/*
 * Prototipos de funciones
 */
void prender_leds(void);
q15_t rand_q15(void);

/**
 * @brief Enciende el LED RGB con el color correspondiente al estado actual.
 */
void prender_leds(void)
{
    switch (f_estado)
    {
        case ESTADO_OFF:
            GPIO_PinWrite(GPIO0, 27, 1);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_IDEN_RAND:
            GPIO_PinWrite(GPIO0, 27, 1);
            GPIO_PinWrite(GPIO0, 10, 0);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_IDEN_CONST:
            GPIO_PinWrite(GPIO0, 27, 0);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_CANCELACION:
            GPIO_PinWrite(GPIO0, 27, 1);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 0);
            break;

        default:
            GPIO_PinWrite(GPIO0, 27, 0);
            GPIO_PinWrite(GPIO0, 10, 0);
            GPIO_PinWrite(GPIO1, 2, 0);
            break;
    }
}

/**
 * @brief Generacion de un numero pseudorandom en formato Q15
 */
q15_t rand_q15(void){
	return (q15_t)((rand() % 32768) - 16384);   // [-0.5, 0.5)
}

/**
 * @brief Rutina de interrupcion de GPIO0.
 *
 * Distingue cual de los dos botones genero la interrupcion, actualiza la
 * maquina de estados, y refleja el cambio en el LED.
 */
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);

    /* Boton SW3: Cambio de valor de mu */
    if ((pin_flags0 & (1U << PIN_START_STOP)) != 0U)
    {
    	if(f_estado != ESTADO_OFF){
    		i_mu = (int16_t)((i_mu + 1U) % CANT_MU_VALUES);
    	}

		#if MODO_DEBUG
            PRINTF("Boton cambio de valor de mu\r\n");
            PRINTF("MU: %d\r\n", i_mu);
        #endif
    }

    /* Boton de cambio de modo*/
    if ((pin_flags0 & (1U << PIN_SWITCH_FREC)) != 0U)
    {
        #if MODO_DEBUG
            PRINTF("Boton cambio de modo presionado\r\n");
        #endif

        f_estado = (estado_muestreo_t)((f_estado + 1U) % CANT_ESTADOS);
    }

    if(f_estado != ESTADO_OFF){
    	// Reinicia el filtro
    	seq++;

   	    arm_lms_init_q15(&lms, NTAPS_ADAPTATIVO, w, lms_state, MU_VALUES[i_mu], BLOCK_SIZE, 0);

   	    // Nueva configuracion
   	    tx_cfg(seq, f_estado, i_mu, MU_VALUES[i_mu]);
    }

    prender_leds();

    GPIO_GpioClearInterruptChannelFlags(GPIO0, pin_flags0, 0U);

#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/**
 * @brief Rutina de interrupcion de match del CTIMER0.
 *
 * @param flags Flags de interrupcion del CTIMER.
 */
void CTIMER0_Callback(uint32_t flags)
{
    (void)flags;

    switch(f_estado){
    	case ESTADO_IDEN_CONST:
    		x = 16384; // 0.5 en Q15
    		break;
    	case ESTADO_IDEN_CONST:
    		x = rand_q15();
    	    break;
    	default:
    		return;
    }

    arm_fir_q15(&planta_inst, &x, &d, 1);
    arm_lms_q15(&lms, &x, &d, &y, &e, BLOCK_SIZE);

    acc += (int64_t)e * e;

    if (++n == 100) {
    	mse = (int32_t)(acc / 100);

    	for (int k = 0; k < 30; k++) {
        	// w en CMSIS-DSP esta invertido respecto a PLANTA_COEF
        	dw[k] = PLANTA_COEF[k] - w[29 - k];
        }

        acc = 0;
        n = 0;
        flag_enviar = 1; // Avisa al main que envie por UART

   }
}

/**
 * @brief Punto de entrada de la aplicacion.
 */
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    // Inicializacion en estado OFF
    f_estado = ESTADO_OFF;
    prender_leds();

    // Inicializacion de los filtros
    arm_fir_init_q15(&planta_inst, PLANTA_NTAPS, PLANTA_COEF, planta_state, BLOCK_SIZE);
    arm_lms_init_q15(&lms, NTAPS_ADAPTATIVO, w, lms_state, MU_VALUES[i_mu], BLOCK_SIZE, 0);

    while (1) {
    	if (flag_enviar) {
    		flag_enviar = 0;

    		// Envio un paquete con formato ya establecido
    		tx_data(seq, mse, dw);
    	}
    }

    return 0;
}
