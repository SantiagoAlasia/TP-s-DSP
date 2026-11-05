/**
 * @file    main.c
 * @brief   Laboratorio 3 - FFT con MCXN947.
 *
 * ============================================================================
 * - Aplicacion de la Transformada Rapida de Fourier
 * - Buffers:
 *     • Entrada: 2 buffers (ping-pong) de FFT_SIZE muestras en formato Q15.
 *     • Salida:  Buffer circular de 512 muestras para lectura por DAC (Bypass).
 * - Control e Interfaz:
 *     • SW2: Conmutacion de tasa de muestreo.
 *     • SW3: Seleccion del modo de operacion(OFF, bypass, FFT).
 *     • LED RGB: Visualizacion de la frecuencia de muestreo.
 * - Salida FFT por UART (una linea de texto por ventana):
 *     FFT,<fs_Hz>,<N>,<mag0>,<mag1>,...,<mag(N/2-1)>\r\n
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

/* Modo Debug */
//#define MODO_DEBUG                1   // 1 = imprime por PRINTF, 0 = no imprime nada

/** Tamanho de la ventana de la FFT: 512, 1024 o 2048. */
#define FFT_SIZE                  512U

/** Cantidad de muestras del buffer circular de salida (DAC). */
#define BUFFER_OUT_SIZE           512U

/** Frecuencia del clock que alimenta al CTIMER0. */
#define TIMER_CLK_HZ              1000000U

/** Cantidad de estados disponibles. */
#define CANT_FREC                 5U

/** Cantidad de modos disponibles. */
#define CANT_MODOS                3U

/** Pin del boton Run/Stop (SW3) dentro del puerto GPIO0. */
#define PIN_START_STOP            6U

/** Pin del boton de cambio de frecuencia (SW2) dentro del puerto GPIO0. */
#define PIN_SWITCH_FREC           23U

/** ID del trigger por software del ADC. */
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

/** Valor de buf_listo cuando no hay ninguna ventana pendiente. */
#define SIN_VENTANA               (-1)

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
volatile frec_muestreo_t f_frec = FREC_8K;

/**
 * Modo de operacion posible.
 */
typedef enum
{
    MODO_OFF      = 1,  /* Adquisicion OFF + Filtrado OFF + DAC ON*/
    MODO_Bypass   = 2,  /* Bypass (No se aplican filtros)*/
    MODO_FFT      = 3   /* Aplicacion de la FFT*/
} modo_muestreo_t;

/** Modo de operacion actual. */
volatile modo_muestreo_t f_modo = MODO_OFF;

/**
 * Valores de matchValue del CTIMER0 para cada frecuencia.
 */
const uint32_t match_values[CANT_FREC] = {124U, 62U, 44U, 22U, 20U};

/* ------------------------- Buffers para la FFT ---------------------------- */

/** Buffers de entrada ping-pong, muestras en formato Q15. */
static q15_t buffer_in[2][FFT_SIZE];

/** Buffer donde se copia la ventana (arm_rfft_q15 modifica su entrada). */
static q15_t fft_entrada[FFT_SIZE];

/** Salida de la FFT real: 2*N valores (parte real e imaginaria intercaladas). */
static q15_t fft_salida[2U * FFT_SIZE];

/** Magnitud del espectro (solo la mitad util: 0 .. fs/2). */
static q15_t fft_mag[FFT_SIZE / 2U];

/** Instancia de la FFT real de CMSIS-DSP. */
static arm_rfft_instance_q15 rfft_inst;

/** Estado de la adquisicion por ventanas. */
static volatile uint8_t  buf_activo = 0U;         /* Buffer que se esta llenando    */
static volatile uint32_t idx_muestra = 0U;        /* Posicion dentro de la ventana  */
static volatile int8_t   buf_listo = SIN_VENTANA; /* Buffer completo para el main   */
static volatile uint32_t ventanas_perdidas = 0U;  /* Ventanas descartadas (overrun) */

/* ------------------------ Buffer de salida (DAC) -------------------------- */

/** Buffer circular de salida, muestras en formato Q15. */
q15_t buffer_out[BUFFER_OUT_SIZE];

/** Punteros para el uso del buffer de salida. */
q15_t * volatile ptr_buffer_lectura_out = buffer_out;
q15_t * volatile ptr_buffer_escritura_out = buffer_out;

/*
 * Prototipos de funciones
 */
void prender_leds(void);
void actualizar_frecuencia_muestreo(void);
void reiniciar_adquisicion(void);
q15_t adc_a_q15(uint16_t valor_adc);
uint16_t q15_a_dac12(q15_t valor_q15);
uint32_t frecuencia_muestreo_hz(void);
void enviar_espectro(void);

/**
 * @brief Enciende el LED RGB con el color correspondiente a la frecuencia de muestreo actual.
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
 * @brief Reconfigura el matchValue del CTIMER0 segun la frecuencia de muestreo actual.
 */
void actualizar_frecuencia_muestreo(void)
{
    if (f_modo == MODO_OFF){
        #if MODO_DEBUG
            PRINTF("ADC detenido (modo OFF): matchValue sin modificar\r\n");
        #endif

        return; // sale de la funcion y no cambia el matchValue
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
 * @brief Descarta la ventana en curso y cualquier ventana pendiente.
 *
 * Se llama al cambiar de modo o de frecuencia, para que una ventana nunca
 * contenga muestras tomadas con dos frecuencias de muestreo distintas.
 */
void reiniciar_adquisicion(void)
{
    idx_muestra = 0U;
    buf_activo  = 0U;
    buf_listo   = SIN_VENTANA;

    ptr_buffer_escritura_out = buffer_out;
    ptr_buffer_lectura_out   = buffer_out;
}

/**
 * @brief Frecuencia de muestreo real (Hz) segun el matchValue actual.
 *
 * El contador va de 0 a matchValue, asi que el periodo es (matchValue + 1) ticks.
 * Se usa para armar el eje de frecuencias en la PC: f(k) = k * fs / N.
 */
uint32_t frecuencia_muestreo_hz(void)
{
    return TIMER_CLK_HZ / (match_values[f_frec] + 1U);
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
 * @brief Envia por UART el espectro calculado
 *
 * Formato: FFT,<fs_Hz>,<N>,<mag0>,...,<mag(N/2-1)>\r\n
 */
void enviar_espectro(void)
{
    PRINTF("FFT,%lu,%lu", (unsigned long)frecuencia_muestreo_hz(), (unsigned long)FFT_SIZE);

    for (uint32_t k = 0U; k < (FFT_SIZE / 2U); k++)
    {
        PRINTF(",%d", (int)fft_mag[k]);
    }

    PRINTF("\r\n");
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

    /* Limpio los flags apenas los leo para no perder un flanco posterior */
    GPIO_GpioClearInterruptChannelFlags(GPIO0, pin_flags0, 0U);

    /* SW3: alterna entre OFF, Bypass y FFT */
    if ((pin_flags0 & (1U << PIN_START_STOP)) != 0U)
    {
        #if MODO_DEBUG
            PRINTF("SW3 presionado\r\n");
        #endif

        f_modo = (modo_muestreo_t)((f_modo % CANT_MODOS) + 1U);
        reiniciar_adquisicion();
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
            reiniciar_adquisicion();
            actualizar_frecuencia_muestreo();
        }
    }

    prender_leds();

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

    /* En modo FFT el DAC no se actualiza (queda con el ultimo valor) */
    if(f_modo == MODO_FFT){
        return;
    }

    /* Saco del buffer de salida la proxima muestra pendiente y la envio al DAC */
    q15_t valor_buffer = *ptr_buffer_lectura_out;
    ptr_buffer_lectura_out++;

    if (ptr_buffer_lectura_out >= (buffer_out + BUFFER_OUT_SIZE))
    {
        ptr_buffer_lectura_out = buffer_out;
    }

    uint16_t valor_dac = q15_a_dac12(valor_buffer);

    DAC_SetData(DAC0, valor_dac);
}

/**
 * @brief Rutina de interrupcion de fin de conversion del ADC0.
 *
 * - MODO_Bypass: la muestra (Q15) va directo al buffer circular de salida.
 * - MODO_FFT:    la muestra se guarda en el buffer ping-pong activo. Al completar
 *                FFT_SIZE muestras se marca la ventana como lista y se cambia de
 *                buffer. El calculo de la FFT lo hace el main.
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

        /* Si el FIFO esta vacio no hay muestra nueva: no uso un valor viejo */
        if (LPADC_GetConvResult(ADC0, &resultado_conversion, ADC_FIFO_RESULT_INDEX))
        {
            q15_t muestra = adc_a_q15(resultado_conversion.convValue);

            if(f_modo == MODO_Bypass){
                /* Escribimos en el buffer de salida el valor en Q15 sin filtrar */
                *ptr_buffer_escritura_out = muestra;
                ptr_buffer_escritura_out++;

                if (ptr_buffer_escritura_out >= (buffer_out + BUFFER_OUT_SIZE))
                {
                    ptr_buffer_escritura_out = buffer_out;
                }
            }else{
                /* MODO_FFT: lleno la ventana activa */
                buffer_in[buf_activo][idx_muestra] = muestra;
                idx_muestra++;

                if (idx_muestra >= FFT_SIZE)
                {
                    idx_muestra = 0U;

                    if (buf_listo == SIN_VENTANA)
                    {
                        /* Entrego la ventana al main y sigo con el otro buffer */
                        buf_listo = (int8_t)buf_activo;
                        buf_activo ^= 1U;
                    }
                    else
                    {
                        /* El main todavia no consumio la anterior: descarto esta
                         * ventana y reutilizo el mismo buffer */
                        ventanas_perdidas++;
                    }
                }
            }
        }
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

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    /* Inicializo la FFT real: N = FFT_SIZE, directa (ifft = 0), con bit reverse (1) */
    if (arm_rfft_init_q15(&rfft_inst, FFT_SIZE, 0U, 1U) != ARM_MATH_SUCCESS)
    {
        PRINTF("Error: FFT_SIZE no soportado por arm_rfft_init_q15\r\n");
        while (1) {}
    }

    // Me aseguro que esten apagados todos los leds.
    prender_leds();

    while (1)
    {
        if ((f_modo == MODO_FFT) && (buf_listo != SIN_VENTANA))
        {
            int8_t b = buf_listo;

            /* Copio la ventana: arm_rfft_q15 destruye su entrada.
             * Despues de la copia libero el buffer para que la ISR pueda reutilizarlo. */
            arm_copy_q15(buffer_in[b], fft_entrada, FFT_SIZE);
            buf_listo = SIN_VENTANA;

            /* FFT real -> 2*N valores (re, im) ; luego magnitud de los N/2 bins utiles */
            arm_rfft_q15(&rfft_inst, fft_entrada, fft_salida);
            arm_cmplx_mag_q15(fft_salida, fft_mag, FFT_SIZE / 2U);

            enviar_espectro();
        }
    }

    return 0;
}
