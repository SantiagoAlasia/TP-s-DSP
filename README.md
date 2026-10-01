# TP2 - Filtros FIR con MCXN947

Trabajo práctico de la materia **Procesamiento Digital de Señales**. Continuación del TP1: implementa el diseño y la aplicación de filtros digitales FIR, procesados por muestras, sobre la señal adquirida con el ADC del microcontrolador **MCXN947** (placa FRDM-MCXN947), reproduciendo el resultado filtrado a través del DAC.

## Objetivo

Diseñar e implementar filtros FIR (pasa bajos, pasa altos, pasa banda y elimina banda), aplicándolos por muestras sobre un buffer de 512 posiciones adquirido a las frecuencias de muestreo del Laboratorio 1 (8K, 16K, 22K, 44K y 48K muestras/segundo), y comparar la señal filtrada contra la original (bypass) mediante osciloscopio.

## Funcionalidades

- **Adquisición heredada del TP1:** el ADC digitaliza la señal de entrada a las mismas 5 velocidades de muestreo (8K/16K/22K/44K/48K S/s), seleccionables con el botón SW2, indicadas con el LED RGB integrado de la placa.
- **Buffer de entrada:** 512 muestras en formato Q15, igual que en el TP1.
- **Filtrado FIR por muestras:** cada muestra adquirida se procesa mediante convolución con los coeficientes del filtro activo, generando la salida en un **buffer distinto al de entrada**.
- **Selección de modo con el botón Run/Stop (SW3):** reutilizando la misma tecla del TP1, cicla circularmente entre OFF y los 5 modos de salida disponibles (4 filtros + bypass):

$$\text{OFF} \rightarrow \text{Pasa Bajos} \rightarrow \text{Pasa Altos} \rightarrow \text{Pasa Banda} \rightarrow \text{Elimina Banda} \rightarrow \text{Bypass} \rightarrow \text{OFF} \rightarrow ...$$

- **Reproducción por DAC:** las muestras del buffer de salida (filtradas o en bypass) se envían al DAC para reconstruir la señal, tal como en el TP1.

## Indicación visual (dos LEDs RGB independientes)

Como ahora conviven dos variables de estado (frecuencia de muestreo y modo de filtrado), se usan dos LEDs RGB separados para evitar ambigüedad — mismo esquema de colores (primarios y combinados) en ambos, para mantener consistencia visual:

- **LED RGB integrado de la placa:** indica la frecuencia de muestreo activa, igual que en el TP1.
- **LED RGB externo (conectado a 3 pines físicos adicionales):** indica el modo de filtrado/bypass activo.

| Estado de filtro | Color LED RGB externo |
|:---:|:---:|
| OFF | Apagado |
| Pasa Bajos | Rojo |
| Pasa Altos | Verde |
| Pasa Banda | Azul |
| Elimina Banda | Rojo + Verde |
| Bypass | Verde + Azul |

## Máquina de estados - Frecuencia de muestreo (heredada del TP1, botón SW2)

| Estado | ADC | Frecuencia | LED RGB integrado |
|:---:|:---:|:---:|:---:|
| 0 | OFF | — | Apagado |
| 1 | ON | 8 kS/s | Rojo |
| 2 | ON | 16 kS/s | Verde |
| 3 | ON | 22 kS/s | Azul |
| 4 | ON | 44 kS/s | Rojo + Verde |
| 5 | ON | 48 kS/s | Verde + Azul |

## Máquina de estados - Modo de filtrado (nueva, botón SW3 / Run-Stop)

| Estado | Modo | LED RGB externo |
|:---:|:---:|:---:|
| 0 | OFF | Apagado |
| 1 | Pasa Bajos (Fc=3600Hz) | Rojo |
| 2 | Pasa Altos (Fc=35Hz) | Verde |
| 3 | Pasa Banda (Fc1=35Hz, Fc2=3500Hz) | Azul |
| 4 | Elimina Banda (Fr=50Hz, BW=15Hz) | Rojo + Verde |
| 5 | Bypass | Verde + Azul |

## Filtros a implementar

| Tipo | Especificación | Atenuación en banda de rechazo |
|:---:|:---:|:---:|
| Pasa Bajos | Fc = 3600 Hz | Astop = 30 dB |
| Pasa Altos | Fc = 35 Hz | Astop = 30 dB |
| Pasa Banda | Fc1 = 35 Hz, Fc2 = 3500 Hz | Astop = 30 dB |
| Elimina Banda | Fr = 50 Hz, BW = 15 Hz | Astop = 25 dB |

## Arquitectura

La arquitectura utilizada esta detallada en el informe.

## Conversión de formatos

- **ADC → Q15:** igual que en el TP1 — el resultado del ADC (sin signo, centrado en la mitad de escala) se recentra restando el offset correspondiente.
- **Filtrado FIR:** convolución en formato Q15 utilizando funciones de CMSIS-DSP (`arm_fir_q15` o equivalente).
- **Q15 → DAC (12 bits):** igual que en el TP1 — reescalado y saturado al rango [0, 4095].

## Hardware utilizado

- Placa **FRDM-MCXN947**
- Entrada analógica: ADC0, canal A0
- Botones de usuario de la placa (SW2: cambio de frecuencia — SW3/Run-Stop: cambio de modo de filtrado)
- LED RGB integrado de la placa (indica frecuencia de muestreo)
- LED RGB externo, conectado a 3 pines GPIO adicionales + resistencias limitadoras (indica modo de filtrado/bypass)

## Herramientas

- **MCUXpresso IDE** + **MCUXpresso SDK**
- **Config Tools** (Pins, Clocks, Peripherals) para la configuración gráfica de ADC0, CTIMER0, DAC0 y los pines del LED RGB externo
- **CMSIS-DSP** (`arm_math.h`) para el tipo de dato Q15 y las funciones de filtrado FIR
- Herramienta de diseño de filtros (Octave `fdatool`) para obtener los coeficientes de cada filtro a partir de las especificaciones

## Diseño de los filtros

Para cada uno de los 4 filtros requeridos se documenta en el informe:
- Método de diseño utilizado (ventaneo)
- Orden del filtro resultante
- Coeficientes obtenidos, convertidos a formato Q15
- Respuesta en frecuencia teórica (magnitud) verificando que cumple Astop en la banda de rechazo

## Estructura del repositorio

```
├── source/
│   └── TP1.c                # Código principal de la aplicación
├── board/                     # Configuración de la placa (generado por Config Tools)
├── drivers/                     # Drivers del SDK utilizados
└── README.md
```

## Informe del Trabajo Práctico


## Cómo compilar

1. Clonar este repositorio
2. Abrir MCUXpresso IDE → **File → Import → Existing Projects into Workspace**
3. Seleccionar la carpeta clonada
4. Compilar (**Build**) y ejecutar (**Debug/Run**) sobre la placa FRDM-MCXN947 conectada por USB
