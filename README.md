# TP2 – Filtros FIR multitasa en FRDM-MCXN947

## Descripción

Este proyecto implementa un sistema de procesamiento digital de señales en tiempo real utilizando el microcontrolador **NXP MCXN947**.

El sistema adquiere una señal analógica mediante el ADC, la procesa digitalmente utilizando filtros FIR (*Finite Impulse Response*) y envía el resultado a través del DAC.

Se implementan cuatro tipos de filtros digitales:

- **Pasa bajos (PB / LPF):** permite el paso de las frecuencias bajas y atenúa las altas.
- **Pasa altos (PA / HPF):** permite el paso de las frecuencias altas y atenúa las bajas.
- **Pasa banda (PBanda / BPF):** permite el paso de un rango determinado de frecuencias.
- **Elimina banda (EB / BSF):** atenúa un rango determinado de frecuencias y permite el paso de las restantes.

Además, el sistema permite seleccionar distintas frecuencias de muestreo y alternar entre los filtros disponibles, el modo bypass y el modo OFF mediante los botones de la placa.

## Características principales

- Procesamiento digital de señales en tiempo real.
- Implementación de filtros FIR en formato de punto fijo **Q15**.
- Arquitectura de filtros multitasa para reducir el costo computacional.
- Frecuencias de muestreo configurables: 8, 16, 22, 44 y 48 kHz.
- Adquisición de muestras mediante ADC.
- Salida de señal procesada mediante DAC de 12 bits.
- Uso de interrupciones para gestionar la adquisición, el procesamiento y la salida.
- Buffers circulares de 512 muestras para entrada y salida.
- Optimización del procesamiento mediante instrucciones DSP del Cortex-M33.

## Hardware y tecnologías

| Elemento | Descripción |
|---|---|
| Microcontrolador | NXP MCXN947 |
| ADC | LPADC |
| DAC | DAC de 12 bits |
| Lenguaje | C |
| Procesamiento DSP | CMSIS-DSP |
| Representación numérica | Q15 |
| Herramienta de diseño de coeficientes | MATLAB |

## Filtros

| Filtro | Especificación | Astop |
|---|---|---|
| PB | fc = 3600 Hz | 30 dB |
| PA | fc = 35 Hz | 30 dB |
| PBanda | 35 Hz a 3500 Hz | 30 dB |
| EB | fr = 50 Hz, BW = 15 Hz | 25 dB |

## Cómo se implementan

Un FIR directo para bandas de pocos hertz a fs altas necesita miles de coeficientes. Por eso:

- **PB:** FIR directo a fs.
- **PA y EB:** `y[n] = x[n-D] - v[n]`, donde `v` sale de un camino angosto que trabaja a 1 kHz:
  `x → HD → ↓M → G → ↑M → M·HD → v`, con `M = fs / 1000`.
- **PBanda:** PA seguido de un pasa bajos directo (LPB).

Según el script, esto usa unos 2,5 mil coeficientes en total (contra ~31 mil de los FIR
directos equiripple) y la peor carga estimada de CPU es 63 % (PBanda a 48 kS/s).

## Arquitectura del sistema

El procesamiento de la señal sigue el siguiente recorrido:

```text
       Señal analógica
              |
              v
         +---------+
         |   ADC   |
         +---------+
              |
              v
       Conversión a Q15
              |
              v
       +--------------+
       | Buffer entrada|
       +--------------+
              |
              v
       +--------------+
       |  Filtro FIR  |
       |   multitasa  |
       +--------------+
              |
              v
       +--------------+
       | Buffer salida|
       +--------------+
              |
              v
       +---------+
       |   DAC   |
       +---------+
              |
              v
       Señal analógica
```

El ADC realiza las conversiones de la señal de entrada. Cada muestra se convierte al formato Q15 y se almacena en el buffer de entrada.

A continuación, se aplica el filtro seleccionado y el resultado se guarda en el buffer de salida. Finalmente, el DAC convierte las muestras procesadas nuevamente a una señal analógica.

La adquisición y la salida se coordinan mediante interrupciones del ADC y del CTIMER.

## Frecuencias de muestreo

El sistema permite trabajar con las siguientes frecuencias:

| Índice | Frecuencia |
|---:|---:|
| 0 | 8 kHz |
| 1 | 16 kHz |
| 2 | 22 kHz |
| 3 | 44 kHz |
| 4 | 48 kHz |

La frecuencia de muestreo se modifica mediante el botón **SW2**. Al cambiarla, se actualiza la configuración del temporizador y, cuando corresponde, se reinicializan los estados del filtro con los coeficientes asociados a la nueva frecuencia.

## Modos de funcionamiento

El botón **SW3** permite recorrer los siguientes modos:

| Modo | Descripción |
|---|---|
| OFF | Detiene la adquisición y el procesamiento. |
| Bypass | Envía la señal sin aplicar filtrado. |
| LPF | Activa el filtro pasa bajos. |
| HPF | Activa el filtro pasa altos. |
| BPF | Activa el filtro pasa banda. |
| BSF | Activa el filtro elimina banda. |

El LED RGB de la placa indica la frecuencia de muestreo seleccionada:

| Frecuencia | Indicador |
|---|---|
| 8 kHz | Rojo |
| 16 kHz | Verde |
| 22 kHz | Azul |
| 44 kHz | Rojo + Verde |
| 48 kHz | Verde + Azul |

En modo OFF, los tres LEDs permanecen apagados.

## Implementación de los filtros FIR

Los filtros FIR calculan cada muestra de salida mediante una suma ponderada de muestras actuales y anteriores:

\[
y[n] = \sum_{k=0}^{N-1} h[k]\,x[n-k]
\]

Donde:

- \(x[n]\) representa la muestra de entrada.
- \(h[k]\) representa los coeficientes del filtro.
- \(N\) es la cantidad de coeficientes.
- \(y[n]\) es la muestra de salida.

Los cálculos se realizan utilizando el formato **Q15**, que permite representar valores fraccionarios mediante enteros de 16 bits con signo.

### Procesamiento multitasa

Para los filtros pasa altos, pasa banda y elimina banda se utiliza una arquitectura multitasa que reduce el costo computacional del procesamiento.

La frecuencia interna del camino de banda angosta se establece en 1 kHz. Para ello, se utilizan etapas de diezmado e interpolación con un factor \(M\) que depende de la frecuencia de muestreo seleccionada:

\[
M = \frac{f_s}{1000}
\]

La estructura general del camino de banda angosta es:

```text
                  +----------------------+
                  |                      |
                  v                      |
Entrada ---> Retardo D              Diezmador
                  |                      |
                  |                      v
                  |                 Filtro G
                  |                      |
                  |                      v
                  |                Interpolador
                  |                      |
                  |                      v
                  |                 Filtro HD
                  |                      |
                  v                      v
                  +-------- Resta -------+
                             |
                             v
                         Salida
```

En términos de implementación, el filtro HD se utiliza tanto en el diezmador como en el interpolador. El procesamiento de sus productos se distribuye entre las muestras para reducir los picos de carga computacional.

El filtro pasa banda se obtiene aplicando un filtro pasa altos seguido de un filtro pasa bajos adicional.

## Organización de archivos

| Archivo | Responsabilidad |
|---|---|
| `main.c` | Inicialización de periféricos, configuración de frecuencias y modos, gestión de botones e interrupciones, adquisición y salida de muestras. |
| `fir_multitasa.h` | Declaración de la API de los filtros y enumeración de los tipos disponibles. |
| `fir_multitasa.c` | Implementación del procesamiento FIR muestra a muestra y de la arquitectura multitasa. |
| `coef_multitasa.h` | Coeficientes FIR, tablas de configuración y parámetros asociados a cada frecuencia de muestreo. |

## Uso en la placa

- **SW3:** cambia el modo: OFF → PB → PA → PBanda → EB → Bypass.
- **SW2:** cambia la fs: 8 → 16 → 22 → 44 → 48 kS/s.
- **LED:** el color indica la fs (rojo 8k, verde 16k, azul 22k, amarillo 44k, cian 48k).
  Apagado en modo OFF.
- **Entrada:** ADC0_A0 (J4[2]), 0 a 3,3 V. **Salida:** DAC0 (J1[4]).

Después de cambiar de filtro o de fs, la salida tarda hasta ~0,2 s en estabilizarse
(el filtro empieza con el estado en cero).

### Funciones principales

La interfaz pública del módulo de filtros se encuentra en `fir_multitasa.h`:

```c
int mt_init(uint8_t idx_fs, mt_tipo_t tipo);

q15_t mt_procesar(q15_t x);

uint32_t mt_retardo_muestras(void);
```

- **`mt_init()`**: selecciona el filtro y la frecuencia de muestreo. También inicializa los estados internos del procesamiento.
- **`mt_procesar()`**: procesa una muestra de entrada en formato Q15 y devuelve la muestra filtrada.
- **`mt_retardo_muestras()`**: devuelve el retardo de grupo del filtro activo, expresado en muestras.

La función `mt_procesar()` debe ejecutarse una vez por cada muestra adquirida.

## Generación de coeficientes

Los coeficientes de los filtros se encuentran definidos en `coef_multitasa.h` y fueron generados mediante el script de MATLAB `disenar_multitasa.m`.

El archivo contiene los coeficientes correspondientes a las cinco frecuencias de muestreo, junto con las tablas de parámetros utilizadas por la implementación.

Los coeficientes se almacenan en formato Q15 y se organizan según el formato requerido por la implementación FIR.

## Compilación y ejecución

El proyecto está desarrollado para el entorno de desarrollo de NXP y utiliza las bibliotecas y configuraciones de periféricos correspondientes al MCXN947.

Para compilar y ejecutar:

1. Abrir el proyecto en el entorno de desarrollo utilizado para el MCXN947.
2. Verificar que estén disponibles las bibliotecas CMSIS-DSP y los drivers de periféricos de NXP.
3. Compilar el proyecto.
4. Descargar el firmware en la placa MCXN947.
5. Conectar la señal analógica de entrada al canal correspondiente del ADC.
6. Observar la señal procesada en la salida del DAC.

> **Nota:** la configuración de pines, relojes y periféricos depende de los archivos generados por el proyecto de NXP (`board`, `peripherals`, `pin_mux` y `clock_config`).

## Consideraciones de implementación

- El procesamiento se realiza muestra a muestra.
- Los estados internos de los filtros se reinician al cambiar su configuración.
- Los filtros utilizan acumuladores de 64 bits para realizar las operaciones de convolución.
- Se emplea una historia duplicada de muestras para evitar copiar datos durante el procesamiento FIR.
- La implementación utiliza instrucciones DSP del Cortex-M33 para optimizar las operaciones de multiplicación y acumulación.
- Si la inicialización del filtro falla, la función `mt_procesar()` deja pasar la muestra sin filtrar.

## Informe del Trabajo Práctico

https://docs.google.com/document/d/1ottVzcl4JmBFpiOeFQYr0KK9VTofG4uatPOUq4b3vT8/edit?usp=sharing

