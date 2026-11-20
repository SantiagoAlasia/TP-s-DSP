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
#include <stdlib.h>
#include <string.h>
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

/* Bypass: 1 = el DAC saca la muestra cruda del ADC (sirve para verificar la parte analogica) */
#define MODO_BYPASS               0

/** Pin del boton de cambio de valor de mu (SW3) dentro del puerto GPIO0. */
#define PIN_START_STOP            6U

/** Pin del boton de cambio de modo (SW2) dentro del puerto GPIO0. */
#define PIN_SWITCH_FREC           23U

/** Cantidad de estados de la FSM. */
#define CANT_ESTADOS       	      4U

/* Mascara para el Trigger 0*/
#define ADC_SW_TRIGGER_ID         1U

/** Indice de resultado leido del FIFO del ADC. */
#define ADC_FIFO_RESULT_INDEX     0U

/** Limites de representacion del DAC de 12 bits. */
#define DAC_VALUE_MAX             4095U
#define DAC_VALUE_MIN             0U

/** Offset y shift usados en la conversion Q15 <-> valores sin signo. */
#define Q15_OFFSET                32768
#define DAC12_OFFSET              2048
#define Q15_TO_DAC12_SHIFT        4U
#define Q15_VALUE_MAX  		      32767
#define Q15_VALUE_MIN 			  -32768
#define Q15_ESCALA                32768.0f

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
volatile estado_muestreo_t f_estado = ESTADO_OFF;

/** Valores intermedios del identificador (solo los usa el CTimer en los estados 1 y 2) */
q15_t x, d, y, e;

/** Envio de datos pendiente */
volatile uint8_t flag_enviar = 0;

/**
 * Reinicio de experimento pendiente (lo levanta la ISR de los botones, lo ejecuta el main).
 * Asi el reinicio nunca interrumpe al main en medio de arm_lms_f32().
 */
volatile uint8_t flag_reset = 0;

/** Ultima muestra del ADC en formato Q15 (la escribe la ISR del ADC, la lee el main). */
volatile q15_t muestra_adc = 0;

/** Ultimo codigo de DAC calculado (lo escribe el main, lo saca el CTimer en el proximo tick). */
volatile uint16_t dac_val = DAC12_OFFSET;   /* mitad de escala */

/** Hay una muestra nueva del ADC para procesar. */
volatile uint8_t nueva_muestra_adc = 0;

static uint32_t g_seed = 123456789U;

/*
 * Prototipos de funciones
 */
void prender_leds(void);
void reiniciar_experimento(void);
q15_t adc_a_q15(uint16_t valor_adc);
uint16_t q15_a_dac12(q15_t valor_q15);
q15_t rand_gauss_q15(void);
void procesar_cancelacion(void);

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
 * @brief Reinicia el experimento: coeficientes adaptativos en cero, estados de los
 *        filtros limpios y acumuladores en cero.
 *
 * arm_lms_init_q15() / arm_lms_init_f32() limpian el estado pero NO los coeficientes,
 * por eso se ponen en cero a mano. Sin esto, un experimento nuevo arranca con los
 * coeficientes del anterior y no se puede comparar random contra constante.
 */
void reiniciar_experimento(void)
{
    /* Planta / correlador (Q15) */
    arm_fir_init_q15(&planta_inst, PLANTA_NTAPS, PLANTA_COEF, planta_state, BLOCK_SIZE);

    /* Parte 1: LMS Q15 */
    memset(w, 0, sizeof(w));
    arm_lms_init_q15(&lms, NTAPS_ADAPTATIVO, w, lms_state, MU_VALUES[i_mu], BLOCK_SIZE, 0);

    /* Parte 2: LMS float */
    memset(w_f, 0, sizeof(w_f));
    arm_lms_init_f32(&lms_f, NTAPS_ADAPTATIVO, w_f, lms_state_f, MU_CANC[i_mu], BLOCK_SIZE);

    acc = 0;
    n = 0;
    flag_enviar = 0;
    nueva_muestra_adc = 0;
    dac_val = DAC12_OFFSET;
}

/**
 * @brief Convierte un resultado crudo del ADC (16 bits, sin signo) a formato Q15.
 * @param valor_adc Valor sin signo devuelto por el ADC (0..65535).
 * @return Valor equivalente en formato Q15 (con signo, centrado en 0).
 */
q15_t adc_a_q15(uint16_t valor_adc)
{
    return (q15_t)((int32_t)valor_adc - Q15_OFFSET);
}

/**
 * @brief Convierte un valor en formato Q15 a un codigo de 12 bits para el DAC.
 * @param valor_q15 Valor en formato Q15 (con signo, centrado en 0).
 * @return Codigo de 12 bits sin signo (0..4095).
 */
uint16_t q15_a_dac12(q15_t valor_q15)
{
    int32_t valor = ((int32_t)valor_q15 >> Q15_TO_DAC12_SHIFT) + DAC12_OFFSET;

    if (valor > (int32_t)DAC_VALUE_MAX)
    {
        valor = (int32_t)DAC_VALUE_MAX;
    }
    else if (valor < (int32_t)DAC_VALUE_MIN)
    {
        valor = (int32_t)DAC_VALUE_MIN;
    }

    return (uint16_t)valor;
}

/**
 * @brief Genera ruido blanco gaussiano (AWGN) en formato Q15.
 *        Aproximacion por Teorema del Limite Central (suma de 12 uniformes).
 *        Desviacion estandar sigma ~= 0.125 (4096 en Q15), media = 0.
 */
q15_t rand_gauss_q15(void)
{
    int32_t suma = 0;

    for (int i = 0; i < 12; i++) {
        // Generador LCG rapido de 32 bits
        g_seed = g_seed * 1664525U + 1013904223U;
        // Toma los 16 bits altos con signo: rango [-32768, 32767] ([-1, 1) en Q15)
        suma += (int16_t)(g_seed >> 16);
    }

    return (q15_t)__SSAT(suma >> 4, 16);
}

/**
 * @brief Rutina de interrupcion de GPIO0.
 *
 * Distingue cual de los dos botones genero la interrupcion, actualiza la
 * maquina de estados y el LED. El reinicio del experimento lo hace el main.
 */
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);

    /* Boton SW3: Cambio de valor de mu */
    if ((pin_flags0 & (1U << PIN_START_STOP)) != 0U)
    {
    	if (f_estado != ESTADO_OFF) {
    		i_mu = (uint8_t)((i_mu + 1U) % CANT_MU_VALUES);
    	}

		#if MODO_DEBUG
            PRINTF("Boton cambio de valor de mu\r\n");
            PRINTF("MU: %d\r\n", i_mu);
        #endif
    }

    /* Boton SW2: cambio de modo */
    if ((pin_flags0 & (1U << PIN_SWITCH_FREC)) != 0U)
    {
        #if MODO_DEBUG
            PRINTF("Boton cambio de modo presionado\r\n");
        #endif

        f_estado = (estado_muestreo_t)((f_estado + 1U) % CANT_ESTADOS);
    }

    if (f_estado != ESTADO_OFF) {
       	flag_reset = 1;
    }

    prender_leds();

    GPIO_GpioClearInterruptChannelFlags(GPIO0, pin_flags0, 0U);

#if defined _CORTEX_M && (_CORTEX_M == 4U)
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

    /* Mientras haya un reinicio pendiente no se procesa*/
    if (flag_reset) {
    	return;
    }

    switch (f_estado) {
    	case ESTADO_IDEN_CONST:
    		x = 16384; // 0.5 en Q15
    		break;
    	case ESTADO_IDEN_RAND:
    		x = rand_gauss_q15();
    	    break;
    	case ESTADO_CANCELACION:
    		/* Saco al DAC la muestra calculada en el tick anterior */
    		DAC_SetData(DAC0, dac_val);

    		/* Disparo la conversion del ADC0 */
    		LPADC_DoSoftwareTrigger(ADC0, ADC_SW_TRIGGER_ID);
    		return;
    	default:
    		return;
    }

    // Funcionamiento en comun para ESTADO_IDEN_CONST & ESTADO_IDEN_RAND
    arm_fir_q15(&planta_inst, &x, &d, BLOCK_SIZE);
    arm_lms_q15(&lms, &x, &d, &y, &e, BLOCK_SIZE);

    acc += (int64_t)e * e;

    if (++n == 100) {
    	mse = (int32_t)(acc / 100);

    	for (int k = 0; k < NTAPS_ADAPTATIVO; k++) {
        	dw[k] = PLANTA_COEF[k] - w[k];
        }

        acc = 0;
        n = 0;
        flag_enviar = 1; // Avisa al main que envie por UART
   }
}

/**
 * @brief Rutina de interrupcion de fin de conversion del ADC0.
 *
 * Si estamos en ESTADO_CANCELACION, convierte el resultado a Q15 y avisa al main
 * que hay una muestra nueva para procesar.
 */
void ADC0_IRQHANDLER(void)
{
	uint32_t trigger_status_flag;
	uint32_t status_flag;
    static lpadc_conv_result_t resultado_conversion;

    trigger_status_flag = LPADC_GetTriggerStatusFlags(ADC0_PERIPHERAL);
	status_flag = LPADC_GetStatusFlags(ADC0_PERIPHERAL);

	LPADC_ClearTriggerStatusFlags(ADC0_PERIPHERAL, trigger_status_flag);
	LPADC_ClearStatusFlags(ADC0_PERIPHERAL, status_flag);

	// Solo guardamos en resultado de la operacion si estamos en ESTADO_CANCELACION
	if (f_estado == ESTADO_CANCELACION) {
		LPADC_GetConvResult(ADC0, &resultado_conversion, ADC_FIFO_RESULT_INDEX);

	    /* Muestra cruda del ADC en Q15 */
	    muestra_adc = adc_a_q15(resultado_conversion.convValue);
	    nueva_muestra_adc = 1;
	}
    /* Errata ARM 838869 (Cortex-M4/M4F): posible vectorizacion incorrecta de
     * interrupcion en un store immediate que se solapa con el retorno. */
	#if defined _CORTEX_M && (_CORTEX_M == 4U)
    	__DSB();
	#endif
}

/**
 * @brief Procesa una muestra de la cancelacion de ruido (Parte 2).
 */
void procesar_cancelacion(void)
{
	q15_t xn, x1n, dn, en;

    /* Ruido de referencia y señal del ADC */
    xn = rand_gauss_q15();
    q15_t signal = muestra_adc;

    /* Correlador FIR: genera el ruido que se suma a la senoidal */
    arm_fir_q15(&planta_inst, &xn, &x1n, BLOCK_SIZE);

    /* Señal deseada: senoidal + ruido correlacionado (con saturacion) */
    int32_t suma = (int32_t)x1n + (int32_t)signal;

    if (suma > Q15_VALUE_MAX) {
    	suma = Q15_VALUE_MAX;
    }
    else if (suma < Q15_VALUE_MIN) {
    	suma = Q15_VALUE_MIN;
    }

    dn = (q15_t)suma;

    /* Filtro adaptativo en float (evita el sesgo por truncamiento de arm_lms_q15) */
    float32_t xf = (float32_t)xn / Q15_ESCALA;
    float32_t df = (float32_t)dn / Q15_ESCALA;
    float32_t yf, ef;

    arm_lms_f32(&lms_f, &xf, &df, &yf, &ef, BLOCK_SIZE);

    /* Error = salida del cancelador, de vuelta a Q15 */
    en = (q15_t)__SSAT((int32_t)(ef * Q15_ESCALA), 16);

#if MODO_BYPASS
    (void)en;
    dac_val = q15_a_dac12(signal);
#else

    /* Salida del cancelador: el CTimer la saca al DAC en el proximo tick */
    dac_val = q15_a_dac12(en);
#endif
}

/**
 * @brief Punto de entrada de la aplicacion.
 */
int main(void){
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
    reiniciar_experimento();

    while (1) {
     	if (flag_reset) {
     		/* Nuevo experimento: se reinicia sin que el CTimer use el filtro a medio reiniciar */
     		__disable_irq();
     		seq++;
     		reiniciar_experimento();
     		flag_reset = 0;
     		__enable_irq();

       		/* Nueva configuracion (etapa y mu). En cancelacion se informa el mu float en Q15. */
     		int16_t mu_q15 = (f_estado == ESTADO_CANCELACION)
     				? (int16_t)(MU_CANC[i_mu] * Q15_ESCALA)
     				: MU_VALUES[i_mu];

        	tx_cfg(seq, (uint8_t)f_estado, i_mu, mu_q15);
        }

        if (flag_enviar) {
        	flag_enviar = 0;

        	// Envio un paquete con formato ya establecido
        	tx_data(seq, mse, dw);
        }

        if (f_estado == ESTADO_CANCELACION && nueva_muestra_adc) {
        	/* Limpiar ANTES de procesar para no perder una muestra que llegue en el medio */
            nueva_muestra_adc = 0;
            procesar_cancelacion();
        }
    }

    return 0;
}
