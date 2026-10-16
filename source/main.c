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
 * - Muestreo: Digitalización analógica a 8, 16, 22, 44.1 y 48 kSps.
 * - Buffers:
 *     • Entrada: Buffer circular de 512 muestras en formato Q15.
 *     • Salida:  Buffer de almacenamiento procesado para lectura por DAC.
 * - Control e Interfaz:
 *     • SW2: Conmutación de tasa de muestreo.
 *     • SW3: Selección dinámica del filtro digital aplicado. (o bypass)
 *     • Touch Pad: Run/Stop del proceso de adquisición y filtrado.
 *     • LED RGB: Visualización del estado del sistema.
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
#include "fsl_powerquad.h"
#include "fsl_powerquad_cmsis.h"

/** Cantidad de muestras del buffer circular. */
#define BUFFER_SIZE               512U

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

/** Buffer circular de entrada, muestras en formato Q15. */
q15_t buffer_in[BUFFER_SIZE];

/** Buffer circular de salida, muestras en formato Q15. */
q15_t buffer_out[BUFFER_SIZE];

/** Puntero para el uso del buffer de entrada. */
q15_t *ptr_buffer_in = buffer_in;

/** Punteros para el uso del buffer de salida. */
q15_t *ptr_buffer_lectura_out = buffer_out;
q15_t *ptr_buffer_escritura_out = buffer_out;

/** Instancia del filtro (CMSIS-DSP / PowerQuad).*/
static arm_fir_instance_q15 fir_inst;
q15_t fir_state[NUM_TAPS_PB_8K];

/*
 * Prototipos de funciones
 */
void prender_leds(void);
q15_t adc_a_q15(uint16_t valor_adc);
uint16_t q15_a_dac12(q15_t valor_q15);

/**
 * @brief Enciende el LED RGB con el color correspondiente al estado actual.
 */
void prender_leds(void)
{
	GPIO_PinWrite(GPIO0, 27, 1);
    GPIO_PinWrite(GPIO0, 10, 0);
    GPIO_PinWrite(GPIO1, 2, 1);
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
 * De momento no hace nada, lo dejo para no modificar la config de perifericos.
 */
void GPIO0_INT_0_IRQHANDLER(void)
{
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);

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

    /* Saco del buffer la proxima muestra pendiente y la envio al DAC */
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
 */
void ADC0_IRQHANDLER(void)
{
    uint32_t trigger_status_flag;
    uint32_t status_flag;
    static lpadc_conv_result_t resultado_conversion;

    trigger_status_flag = LPADC_GetTriggerStatusFlags(ADC0);
    status_flag = LPADC_GetStatusFlags(ADC0);

    LPADC_ClearTriggerStatusFlags(ADC0, trigger_status_flag);
    LPADC_ClearStatusFlags(ADC0, status_flag);


    LPADC_GetConvResult(ADC0, &resultado_conversion, ADC_FIFO_RESULT_INDEX);

    /* Muestra cruda en el buffer de entrada */
    *ptr_buffer_in = adc_a_q15(resultado_conversion.convValue);

    arm_fir_q15(&fir_inst, ptr_buffer_in, ptr_buffer_escritura_out, 1U);

    ptr_buffer_in++;
    ptr_buffer_escritura_out++;

    /* Apuntamos al inicio de los buffers*/
    if (ptr_buffer_in >= (buffer_in + BUFFER_SIZE))
    {
    	#if MODO_DEBUG
        	PRINTF("Ultima Dir de ptr_buffer: %p\r\n", (void*)ptr_buffer_in);
      	#endif

        ptr_buffer_in = buffer_in;
        ptr_buffer_escritura_out = buffer_out;

        #if MODO_DEBUG
           PRINTF("Dir inicial de ptr_buffer: %p\r\n", (void*)ptr_buffer_in);
        #endif
    }

    #if MODO_DEBUG
    	PRINTF("Conversion: %d\r\n", *(ptr_buffer_in - 1));
    #endif

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

    // Al colocarlo aca nos aseguramos de instanciar antes de que algun periferico pueda interumpir el ciclo de ejecucion.
    //PQ_Init(POWERQUAD);
    //arm_fir_init_q15(&fir_inst, NUM_TAPS_PB_8K, (q15_t *)pb_8k, fir_state, 1U);

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    prender_leds();

    while (1) {}

    return 0;
}
