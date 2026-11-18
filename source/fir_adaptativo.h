/*
 * fir_adaptativo.h - Estructuras y Datos nesesarios para el filtro.
 */
#ifndef FIR_ADAPTATIVO_H_
#define FIR_ADAPTATIVO_H_

/** Cantidad de valores posibles para MU disponibles. */
#define CANT_MU_VALUES    	      6U

/** Cantidad de coeficientes del filtro adaptativo */
#define NTAPS_ADAPTATIVO		  30

/* Tabla de mu en Q15 (SW2 avanza): 0.01  0.05  0.1  0.25  0.5  0.9 */
const int16_t MU_VALUES[CANT_MU_VALUES] = { 328, 1638, 3277, 8192, 16384, 29491 };

/* Indice del valor de MU actual*/
static uint8_t i_mu = 0;

/** Coeficientes Adaptativos */
static q15_t w[NTAPS_ADAPTATIVO];

/** Estado del filtro */
static q15_t lms_state[NTAPS_ADAPTATIVO];

/** Instancia del filtro */
static arm_lms_instance_q15 lms;

/** Numero de experimento */
static uint32_t seq = 0;

/* Acumulador para el calculo del MSE */
static int64_t acc = 0;

/** Cantidad de Muestras de la ventana actual*/
static uint32_t n = 0;

/** Valor del MSE */
static int32_t mse;

/** Arreglo con la diferencia entre los coeficientes definidos y los estimados: d[i] = coef[i] - w[i]*/
static int16_t dw[NTAPS_ADAPTATIVO];

#endif /* FIR_ADAPTATIVO_H_ */
