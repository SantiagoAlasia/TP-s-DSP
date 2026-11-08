/**
 * @file    fir_multitasa.h
 * @brief   Filtros FIR multitasa (PB, PA, PBanda, EB) en Q15 para el MCXN947.
 *
 * Usa los coeficientes que calculamos mediante el script de MATLAB (coef_multitasa.h).
 * Uso:
 *     mt_init(f_frec, MT_PA);          // al cambiar de filtro o de fs
 *     y = mt_procesar(x);              // una vez por muestra (ISR del ADC)
 */
#ifndef FIR_MULTITASA_H_
#define FIR_MULTITASA_H_

#include <stdint.h>
#include "arm_math.h"

/** Tipo de filtro (mismo orden que en disenar_multitasa.m). */
typedef enum
{
    MT_PB     = 0,   /* Pasa bajos    (LPF) */
    MT_PA     = 1,   /* Pasa altos    (HPF) */
    MT_PBANDA = 2,   /* Pasa banda    (BPF) */
    MT_EB     = 3    /* Elimina banda (BSF) */
} mt_tipo_t;

/**
 * @brief Selecciona filtro y frecuencia de muestreo y reinicia todos los estados.
 * @param idx_fs Indice de fs: 0=8k, 1=16k, 2=22k, 3=44k, 4=48k (igual que frec_muestreo_t).
 * @param tipo   Filtro a aplicar.
 * @return 0 si salio bien, distinto de 0 si algun argumento o coeficiente es invalido
 *         (en ese caso mt_procesar() deja pasar la muestra sin filtrar).
 */
int mt_init(uint8_t idx_fs, mt_tipo_t tipo);

/**
 * @brief Procesa UNA muestra. Llamar una vez por cada muestra del ADC.
 * @param x Muestra de entrada en Q15.
 * @return Muestra filtrada en Q15 (con el retardo propio del filtro).
 */
q15_t mt_procesar(q15_t x);

/** @brief Retardo de grupo del filtro activo, en muestras. */
uint32_t mt_retardo_muestras(void);

#endif /* FIR_MULTITASA_H_ */
