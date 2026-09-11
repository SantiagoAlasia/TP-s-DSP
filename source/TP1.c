/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    TP1.c
 * @brief   Laboratorio 1 - Muestreo con MCXN947.
 *
 * Digitaliza una señal analógica a distintas frecuencias de muestreo
 * (8K/16K/22K/44K/48K S/s), almacenando las muestras en un buffer
 * circular en formato Q15, y reproduce la señal adquirida a través
 * del DAC. El cambio de frecuencia y el Run/Stop se controlan con
 * botones de la placa; el estado activo se indica con el LED RGB.
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "arm_math.h"
#include "fsl_debug_console.h"
#include "fsl_lpadc.h"

/* Modo Debug*/
#define MODO_DEBUG   			 1   // 1 = imprime por PRINTF, 0 = no imprime nada

/** Pin del botón Run/Stop (SW3) dentro del puerto GPIO0. */
#define PIN_START_STOP           6U

/** Pin del botón de cambio de frecuencia (SW2) dentro del puerto GPIO0. */
#define PIN_SWITCH_FREC          23U

/** Cantidad de muestras del buffer circular. */
#define BUFFER_SIZE              512U

/** Cantidad de estados de frecuencia disponibles (sin contar el estado OFF). */
#define CANT_ESTADOS_FREC        5U

/** Número de trigger de software usado para disparar el ADC1. */
#define ADC_SW_TRIGGER_ID        1U

/** Índice de resultado leído del FIFO del ADC. */
#define ADC_FIFO_RESULT_INDEX    0U

/** Límites de representación del DAC de 12 bits. */
#define DAC_VALUE_MAX            4095U
#define DAC_VALUE_MIN            0U

/** Offset y shift usados en la conversión Q15 <-> valores sin signo. */
#define Q15_OFFSET               32768
#define DAC12_OFFSET             2048
#define Q15_TO_DAC12_SHIFT       4U

/**
 * Estados de la máquina de estados principal.
 * ESTADO_OFF representa ADC apagado y LEDs apagados;
 * los demás representan una frecuencia de muestreo y color de LED distintos.
 */
typedef enum
{
    ESTADO_OFF   = 0,  /* ADC OFF, LEDs OFF */
    ESTADO_8K    = 1,  /* 8 kS/s  - LED Rojo */
    ESTADO_16K   = 2,  /* 16 kS/s - LED Verde */
    ESTADO_22K   = 3,  /* 22 kS/s - LED Azul */
    ESTADO_44K   = 4,  /* 44 kS/s - LED Rojo + Verde */
    ESTADO_48K   = 5   /* 48 kS/s - LED Verde + Azul */
} estado_muestreo_t;

/** Estado actual de la máquina de estados. */
estado_muestreo_t f_estado = ESTADO_OFF;

/** Último estado activo antes de pasar a OFF (para poder reanudar con Run/Stop). */
estado_muestreo_t f_estado_anterior = ESTADO_8K;

/** Buffer circular de muestras en formato Q15. */
q15_t buffer[BUFFER_SIZE];

/** Punteros para la lectura y escritura del buffer. */
q15_t *ptr_buffer_escritura = buffer;
q15_t *ptr_buffer_lectura = buffer;

/**
 * Valores de matchValue del CTIMER0 para cada estado de frecuencia
 */
const uint32_t match_values[CANT_ESTADOS_FREC] = {124U, 62U, 44U, 22U, 20U};

/*
 * Prototipos de funciones
 */
void prender_leds(void);
void actualizar_frecuencia_muestreo(void);
q15_t adc_a_q15(uint16_t valor_adc);
uint16_t q15_a_dac12(q15_t valor_q15);

/**
 * @brief Enciende el LED RGB con el color correspondiente al estado actual.
 */
void prender_leds(void)
{
    switch (f_estado)
    {
        case ESTADO_OFF:
            /* Todos apagados (activo en bajo) */
            GPIO_PinWrite(GPIO0, 27, 1);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_8K:
            /* Rojo */
            GPIO_PinWrite(GPIO0, 27, 1);
            GPIO_PinWrite(GPIO0, 10, 0);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_16K:
            /* Verde */
            GPIO_PinWrite(GPIO0, 27, 0);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_22K:
            /* Azul */
            GPIO_PinWrite(GPIO0, 27, 1);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 0);
            break;

        case ESTADO_44K:
            /* Rojo + Verde */
            GPIO_PinWrite(GPIO0, 27, 0);
            GPIO_PinWrite(GPIO0, 10, 0);
            GPIO_PinWrite(GPIO1, 2, 1);
            break;

        case ESTADO_48K:
            /* Verde + Azul */
            GPIO_PinWrite(GPIO0, 27, 0);
            GPIO_PinWrite(GPIO0, 10, 1);
            GPIO_PinWrite(GPIO1, 2, 0);
            break;

        default:
            /* Estado inesperado: todos encendidos como indicador de error */
            GPIO_PinWrite(GPIO0, 27, 0);
            GPIO_PinWrite(GPIO0, 10, 0);
            GPIO_PinWrite(GPIO1, 2, 0);
            break;
    }
}

/**
 * @brief Reconfigura el matchValue del CTIMER0 según el estado actual.
 */
void actualizar_frecuencia_muestreo(void)
{
    if (f_estado == ESTADO_OFF)
    {
        PRINTF("ADC detenido (estado OFF): matchValue sin modificar\r\n");
        return;
    }

    ctimer_match_config_t match_config = CTIMER0_Match_3_config;
    match_config.matchValue = match_values[f_estado - 1];

    CTIMER_StopTimer(CTIMER0_PERIPHERAL);
    CTIMER0_PERIPHERAL->TC = 0U;   // Reinicio del contador
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_3_CHANNEL, &match_config);
    CTIMER_StartTimer(CTIMER0_PERIPHERAL);

	#if MODO_DEBUG
    	PRINTF("Match value actualizado: %lu\r\n", match_values[f_estado - 1]);
	#endif
}

/**
 * @brief Convierte un resultado crudo del ADC a formato Q15.
 * @param valor_adc Valor sin signo devuelto por el ADC.
 * @return Valor equivalente en formato Q15 (con signo, centrado en 0).
 */
q15_t adc_a_q15(uint16_t valor_adc)
{
    return (q15_t)((int32_t)valor_adc - Q15_OFFSET);
}

/**
 * @brief Convierte un valor en formato Q15 a un código de 12 bits para el DAC.
 * @param valor_q15 Valor en formato Q15 (con signo, centrado en 0).
 * @return Código de 12 bits sin signo.
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
 * @brief Rutina de interrupción de GPIO0.
 *
 * Distingue cuál de los dos botones generó la interrupción,
 * actualiza la máquina de estados, y refleja actualiza el LED indicador
 * y la frecuencia de muestreo.
 */
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);

    /* Boton Run/Stop: alterna entre ESTADO_OFF y el ultimo estado activo */
    if ((pin_flags0 & (1U << PIN_START_STOP)) != 0U)
    {
		#if MODO_DEBUG
        	PRINTF("Boton Run/Stop presionado\r\n");
		#endif

        if (f_estado != ESTADO_OFF)
        {
            f_estado_anterior = f_estado;
            f_estado = ESTADO_OFF;
        }
        else
        {
            f_estado = f_estado_anterior;
            f_estado_anterior = ESTADO_OFF;
        }
    }

    /* Boton de cambio de frecuencia: ciclo circular 1 -> 5 -> 1, solo si esta corriendo */
    if ((pin_flags0 & (1U << PIN_SWITCH_FREC)) != 0U)
    {
		#if MODO_DEBUG
        	PRINTF("Boton cambio de frecuencia presionado\r\n");
		#endif

        if (f_estado != ESTADO_OFF)
        {
            f_estado = (estado_muestreo_t)((f_estado % CANT_ESTADOS_FREC) + 1U);
        }
    }

    actualizar_frecuencia_muestreo();
    prender_leds();

    GPIO_GpioClearInterruptChannelFlags(GPIO0, pin_flags0, 0U);

#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/**
 * @brief Rutina de interrupción de match del CTIMER0.
 *
 * Dispara la conversión del ADC1 por software, y en simultáneo envía al
 * DAC la próxima muestra pendiente del buffer circular.
 *
 * @param flags Flags de interrupción del CTIMER (no utilizados).
 */
void CTIMER0_Callback(uint32_t flags)
{
    (void)flags;

    /* Disparo la conversion del ADC1 */
    LPADC_DoSoftwareTrigger(ADC1, ADC_SW_TRIGGER_ID);

    /* Envio al DAC la proxima muestra pendiente del buffer */
    q15_t valor_buffer = *ptr_buffer_lectura;
    ptr_buffer_lectura++;

    if (ptr_buffer_lectura >= (buffer + BUFFER_SIZE))
    {
        ptr_buffer_lectura = buffer;
    }

    uint16_t valor_dac = q15_a_dac12(valor_buffer);
    DAC_SetData(DAC0, valor_dac);
}

/**
 * @brief Rutina de interrupción de fin de conversión del ADC1.
 *
 * Si f_estado != ESTADO_OFF, convierte el resultado a formato Q15
 * y lo almacena en la siguiente posición del buffer circular.
 */
void ADC1_IRQHANDLER(void)
{
    uint32_t trigger_status_flag;
    uint32_t status_flag;
    static lpadc_conv_result_t resultado_conversion;

    trigger_status_flag = LPADC_GetTriggerStatusFlags(ADC1_PERIPHERAL);
    status_flag = LPADC_GetStatusFlags(ADC1_PERIPHERAL);

    LPADC_ClearTriggerStatusFlags(ADC1_PERIPHERAL, trigger_status_flag);
    LPADC_ClearStatusFlags(ADC1_PERIPHERAL, status_flag);

    if (f_estado != ESTADO_OFF)
    {
        LPADC_GetConvResult(ADC1, &resultado_conversion, ADC_FIFO_RESULT_INDEX);

        *ptr_buffer_escritura = adc_a_q15(resultado_conversion.convValue);
        ptr_buffer_escritura++;

        if (ptr_buffer_escritura >= (buffer + BUFFER_SIZE))
        {
			#if MODO_DEBUG
        		PRINTF("Ultima Dir de ptr_buffer: %p\r\n", (void*)ptr_buffer_escritura);
			#endif

            ptr_buffer_escritura = buffer;

			#if MODO_DEBUG
            	PRINTF("Dir inicial de ptr_buffer: %p\r\n", (void*)ptr_buffer_escritura);
			#endif
        }

		#if MODO_DEBUG
        	PRINTF("Conversion: %d\r\n", *(ptr_buffer_escritura - 1));
		#endif
    }

    /* Errata ARM 838869 (Cortex-M4/M4F): posible vectorizacion incorrecta de
     * interrupcion en un store immediate que se solapa con el retorno. */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/**
 * @brief Punto de entrada de la aplicación.
 */
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    while (1){}

    return 0;
}
