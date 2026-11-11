/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    TP2.c
 * @brief   Laboratorio 2 - Filtro FIR con MCXN947.
 *
 * ============================================================================
 * - Muestreo: Digitalización analógica a 8, 16, 22, 44 y 48 kSps.
 * - Buffers:
 *     • Entrada: Buffer circular de 512 muestras en formato Q15.
 *     • Salida:  Buffer de almacenamiento procesado para lectura por DAC.
 * - Control e Interfaz:
 *     • SW2: Conmutación de tasa de muestreo.
 *     • SW3: Selección dinámica del filtro digital aplicado, bypass o stop.
 *     • LED RGB: Visualización de la frecuencia de muestreo.
 * ============================================================================
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
#include "fir_multitasa.h"   /* funcionalidades para los filtros */

/* Modo Debug */
//#define MODO_DEBUG                1   // 1 = imprime por PRINTF, 0 = no imprime nada

/** Cantidad de muestras del buffer circular. */
#define BUFFER_SIZE               512U

/** Cantidad de estados disponibles. */
#define CANT_FREC                 5U

/** Cantidad de modos disponibles. */
#define CANT_MODOS                6U

/** Pin del boton Run/Stop (SW3) dentro del puerto GPIO0. */
#define PIN_START_STOP            6U

/** Pin del boton de cambio de frecuencia (SW2) dentro del puerto GPIO0. */
#define PIN_SWITCH_FREC           23U

/** Mascara para el Trigger 0*/
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

/**
 * Frecuencias de muestreo posibles.
 */
typedef enum
{
    FREC_8K    = 0,  /* 8  kS/s - LED Rojo*/
    FREC_16K   = 1,  /* 16 kS/s - LED Verde */
    FREC_22K   = 2,  /* 22 kS/s - LED Azul */
    FREC_44K   = 3,  /* 44 kS/s - LED Rojo + Verde */
    FREC_48K   = 4   /* 48 kS/s - LED Verde + Azul */
} frec_muestreo_t;

/** Frecuencia actual. */
frec_muestreo_t f_frec = FREC_8K;

/**
 * Modo de operacion posible.
 */
typedef enum
{
    MODO_OFF      = 1,  /* Adquisicion OFF + Filtrado OFF + DAC ON*/
    MODO_LPF      = 2,  /* Filtro Pasa Bajo*/
    MODO_HPF      = 3,  /* Filtro Pasa Alto */
    MODO_BPF      = 4,  /* Filtro Pasa Banda */
    MODO_BSF      = 5,  /* Filtro Elimina Banda */
    MODO_Bypass   = 6   /* Bypass (No se aplican filtros)*/
} modo_muestreo_t;

/** Modo de operacion actual. */
modo_muestreo_t f_modo = MODO_OFF;

/** Buffer circular de entrada, muestras en formato Q15. */
q15_t buffer_in[BUFFER_SIZE];

/** Buffer circular de salida, muestras en formato Q15. */
q15_t buffer_out[BUFFER_SIZE];

/** Puntero para el uso del buffer de entrada. */
q15_t *ptr_buffer_in = buffer_in;

/** Punteros para el uso del buffer de salida. */
q15_t *ptr_buffer_lectura_out = buffer_out;
q15_t *ptr_buffer_escritura_out = buffer_out;

/**
 * Valores de matchValue del CTIMER0 para cada frecuencia.
 */
const uint32_t match_values[CANT_FREC] = {18749U, 9374U, 6817U, 3408U, 3124U};

/*
 * Prototipos de funciones
 */
void prender_leds(void);
void actualizar_frecuencia_muestreo(void);
void actualizar_modo_muestreo(void);
q15_t adc_a_q15(uint16_t valor_adc);
uint16_t q15_a_dac12(q15_t valor_q15);

/**
 * @brief Enciende el LED RGB con el color correspondiente a la frecuencia de muestreo actual.
 * 		  En caso de estar en modo OFF apaga los 3 LEDs.
 */
void prender_leds(void)
{
    // Si el modo es apagado => No prendemos ningun LED
    if(f_modo == MODO_OFF)
    {
        GPIO_PinWrite(GPIO0, 27, 1);
        GPIO_PinWrite(GPIO0, 10, 1);
        GPIO_PinWrite(GPIO1, 2,  1);
        return;
    }else{
	    switch (f_frec)
        {
            case FREC_8K:
                GPIO_PinWrite(GPIO0, 27, 1);
                GPIO_PinWrite(GPIO0, 10, 0);
                GPIO_PinWrite(GPIO1, 2,  1);
                break;

            case FREC_16K:
                GPIO_PinWrite(GPIO0, 27, 0);
                GPIO_PinWrite(GPIO0, 10, 1);
                GPIO_PinWrite(GPIO1, 2,  1);
                break;

            case FREC_22K:
                GPIO_PinWrite(GPIO0, 27, 1);
                GPIO_PinWrite(GPIO0, 10, 1);
                GPIO_PinWrite(GPIO1, 2,  0);
                break;

            case FREC_44K:
                GPIO_PinWrite(GPIO0, 27, 0);
                GPIO_PinWrite(GPIO0, 10, 0);
                GPIO_PinWrite(GPIO1, 2,  1);
                break;

            case FREC_48K:
                GPIO_PinWrite(GPIO0, 27, 0);
                GPIO_PinWrite(GPIO0, 10, 1);
                GPIO_PinWrite(GPIO1, 2,  0);
                break;

            default:
                GPIO_PinWrite(GPIO0, 27, 0);
                GPIO_PinWrite(GPIO0, 10, 0);
                GPIO_PinWrite(GPIO1, 2,  0);
                break;
        }
    }
}

/**
 * @brief Siempre que se encuentre en modo != OFF, reconfigura el matchValue del CTIMER0
 * 		  segun la frecuencia de muestreo actual.
 */
void actualizar_frecuencia_muestreo(void)
{
    if (f_modo == MODO_OFF){
        #if MODO_DEBUG
            PRINTF("ADC detenido (modo OFF): matchValue sin modificar\r\n");
        #endif
        return;
    }

    ctimer_match_config_t match_config = CTIMER0_Match_0_config;
    match_config.matchValue = match_values[f_frec];

    CTIMER_StopTimer(CTIMER0_PERIPHERAL);
    CTIMER0_PERIPHERAL->TC = 0U;   // Reinicio del contador
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &match_config);
    CTIMER_StartTimer(CTIMER0_PERIPHERAL);

    #if MODO_DEBUG
        PRINTF("Match value actualizado: %lu\r\n", match_values[f_frec]);
    #endif
}

/**
 * @brief Selecciona el filtro multitasa segun el modo y la frecuencia actuales
 *        y reinicia sus estados. Se llama cada vez que cambia el modo o la fs.
 */
void actualizar_modo_muestreo(void) {
	mt_tipo_t tipo;

	switch (f_modo) {
		case MODO_LPF: tipo = MT_PB;     break;
		case MODO_HPF: tipo = MT_PA;     break;
		case MODO_BPF: tipo = MT_PBANDA; break;
		case MODO_BSF: tipo = MT_EB;     break;
		default: /* MODO_OFF, MODO_Bypass: no se usa el filtro */
			return;
	}

	/* Que el ADC no use el filtro mientras se reconfigura */
	NVIC_DisableIRQ(ADC0_IRQn);

	/* f_frec (FREC_8K..FREC_48K = 0..4) coincide con el indice de fs de filtros_multitasa.h */
	if (mt_init((uint8_t) f_frec, tipo) != 0) {
		#if MODO_DEBUG
	            PRINTF("Fallo la inicializacion del filtro\r\n");
	    #endif
	}

	NVIC_EnableIRQ(ADC0_IRQn);
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
 * @brief Rutina de interrupcion de GPIO0.
 *
 * Distingue cual de los dos botones genero la interrupcion, actualiza las flags
 * y refleja el cambio en el LED y en la frecuencia de muestreo.
 */
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);

    /* SW3: alterna entre ESTADO_OFF, Bypass y los distintos filtros */
    if ((pin_flags0 & (1U << PIN_START_STOP)) != 0U)
    {
        #if MODO_DEBUG
            PRINTF("SW3 presionado\r\n");
        #endif

        f_modo = (modo_muestreo_t)((f_modo % CANT_MODOS) + 1U);
    }

    /* SW2: Boton de cambio de frecuencia*/
    if ((pin_flags0 & (1U << PIN_SWITCH_FREC)) != 0U)
    {
        #if MODO_DEBUG
            PRINTF("Boton cambio de frecuencia presionado\r\n");
        #endif

        if (f_modo != MODO_OFF)
        {
            f_frec = (frec_muestreo_t)((f_frec + 1U) % CANT_FREC);
        }

        actualizar_frecuencia_muestreo();

    }

    actualizar_modo_muestreo();
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

    /* Disparo la conversion del ADC0 */
    LPADC_DoSoftwareTrigger(ADC0, ADC_SW_TRIGGER_ID);

    /* Saco del buffer de salida la proxima muestra pendiente y la envio al DAC */
    q15_t valor_buffer = *ptr_buffer_lectura_out;
    ptr_buffer_lectura_out++;

    if (ptr_buffer_lectura_out >= (buffer_out + BUFFER_SIZE))
    {
        ptr_buffer_lectura_out = buffer_out;
    }

    uint16_t valor_dac = q15_a_dac12(valor_buffer);

    DAC_SetData(DAC0, valor_dac);
}

/**
 * @brief Rutina de interrupcion de fin de conversion del ADC0.
 *
 * Si (f_modo != MODO_OFF && f_modo != MODO_Bypass), convierte el resultado a Q15 y lo guarda en el buffer de entrada.
 * Luego, aplica el filtro activo y guarda el resultado en el buffer de salida.
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

    if(f_modo != MODO_OFF){
        LPADC_GetConvResult(ADC0, &resultado_conversion, ADC_FIFO_RESULT_INDEX);

        /* Muestra cruda en el buffer de entrada */
        *ptr_buffer_in = adc_a_q15(resultado_conversion.convValue);

        if(f_modo == MODO_Bypass){
            /* Escribimos en el buffer de salida el valor en Q15 sin filtrar */
            *ptr_buffer_escritura_out = *ptr_buffer_in;
        }else{
            /* Filtrado de la muestra obtenida */
            *ptr_buffer_escritura_out = mt_procesar(*ptr_buffer_in);
        }

        ptr_buffer_in++;
        ptr_buffer_escritura_out++;

        /* Apuntamos al inicio de los buffers*/
        if (ptr_buffer_in >= (buffer_in + BUFFER_SIZE))
        {
    	    #if MODO_DEBUG
            	PRINTF("Ultima Dir de ptr_buffer_in: %p\r\n", (void*)ptr_buffer_in);
                PRINTF("Ultima Dir de ptr_buffer_escritura_out %p\r\n", (void*)ptr_buffer_escritura_out);
      	    #endif

    	    ptr_buffer_in = buffer_in;
            ptr_buffer_escritura_out = buffer_out;

            #if MODO_DEBUG
                PRINTF("Dir inicial de ptr_buffer_in: %p\r\n", (void*)ptr_buffer_in);
                PRINTF("Dir inicial de ptr_buffer_escritura_out: %p\r\n", (void*)ptr_buffer_escritura_out);
            #endif
    }

    #if MODO_DEBUG
    	PRINTF("Conversion: %d\r\n", *(ptr_buffer_in - 1));
        PRINTF("Filtrado: %d\r\n", *(ptr_buffer_escritura_out - 1));
    #endif
    }

    /* Errata ARM 838869 (Cortex-M4/M4F): posible vectorizacion incorrecta de
     * interrupcion en un store immediate que se solapa con el retorno. */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/**
 * @brief Punto de entrada de la aplicacion.
 */
int main(void)
{
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();

    // Me aseguro que esten apagados todos los leds.
    prender_leds();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    while (1) {}

    return 0;
}
