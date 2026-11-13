# TP3 - FFT en tiempo real con MCXN947

Implementación de la Transformada Rápida de Fourier (FFT) sobre una señal adquirida con el ADC de la **FRDM-MCXN947**. El espectro se transmite por puerto serie y se grafica en la PC con un script de **Octave**.

Este laboratorio parte de la adquisición del Laboratorio 1 (CTIMER + ADC + DAC) y agrega un modo de procesamiento en frecuencia con ventanas de 512, 1024 o 2048 muestras.

## Características

- Adquisición con ADC (LPADC) disparado por **CTIMER0** a 5 frecuencias de muestreo seleccionables.
- Tres modos de operación: **OFF**, **Bypass** (ADC → DAC sin procesar) y **FFT**.
- FFT real en formato **Q15** con CMSIS-DSP (`arm_rfft_q15` + `arm_cmplx_mag_q15`).
- Tamaño de ventana configurable en compilación: **512, 1024 o 2048** muestras.
- Adquisición con **doble buffer (ping-pong)**: las ISR solo guardan muestras y el cálculo se hace fuera de la interrupción.
- Transmisión del espectro por UART (consola de debug) en formato de texto.
- Script de Octave para recibir y graficar el espectro en tiempo real.

## Hardware

| Elemento | Detalle |
|---|---|
| Placa | NXP FRDM-MCXN947 |
| Entrada analógica | ADC0 (LPADC), trigger por software |
| Salida analógica | DAC0 (12 bits) |
| Pulsador SW3 | GPIO0, pin 6: cambia el modo (OFF → Bypass → FFT) |
| Pulsador SW2 | GPIO0, pin 23: cambia la frecuencia de muestreo |
| LED RGB | GPIO0 pin 27, GPIO0 pin 10, GPIO1 pin 2 (activos en bajo) |
| Comunicación | VCOM del depurador MCU-Link (mismo cable USB de programación) |

## Controles

**SW3 - modo de operación** (cicla en este orden):

| Modo | Descripción |
|---|---|
| `MODO_OFF` | Adquisición detenida, LEDs apagados |
| `MODO_Bypass` | La muestra del ADC va al buffer de salida y se reproduce por el DAC |
| `MODO_FFT` | Se acumulan ventanas, se calcula la FFT y se envía por UART. El DAC no se actualiza |

**SW2 - frecuencia de muestreo** (solo fuera de OFF):

| Estado | `matchValue` | fs real (con timer a 1 MHz) | LED |
|---|---|---|---|
| `FREC_8K` | 124 | 8000 Hz | Rojo |
| `FREC_16K` | 62 | 15873 Hz | Verde |
| `FREC_22K` | 44 | 22222 Hz | Azul |
| `FREC_44K` | 22 | 43478 Hz | Rojo + Verde |
| `FREC_48K` | 20 | 47619 Hz | Verde + Azul |

> Las frecuencias reales se calculan como `TIMER_CLK_HZ / (matchValue + 1)`. Los nombres (22K, 44K, etc.) son nominales. Verificar que `TIMER_CLK_HZ` en `main.c` coincida con el clock real del CTIMER0.

## Estructura del proyecto

```
.
├── source/
│   └── main.c            # Firmware (adquisición, FFT y transmisión)
├── octave/
│   └── fft_monitor.m     # Receptor y graficador del espectro
└── README.md
```

El resto del proyecto (pines, clocks, periféricos, SDK) es el generado por MCUXpresso IDE y el Config Tools.

## Cómo funciona

```
CTIMER0 ──(cada 1/fs)──► dispara el ADC
                              │
                              ▼
                  ISR del ADC (muestra lista)
                   │                     │
              MODO_Bypass            MODO_FFT
                   │                     │
         buffer_out circular     buffer_in[0] / buffer_in[1]  (ping-pong)
                   │                     │ ventana completa → buf_listo
         ISR del timer → DAC             ▼
                                   main: copia → FFT → magnitud → UART
```

### Buffers

- **Ping-pong de entrada:** `buffer_in[2][FFT_SIZE]`. Mientras la ISR llena un buffer, el `main` procesa el otro. Las variables `buf_activo`, `idx_muestra` y `buf_listo` coordinan el traspaso.
- **Ventanas perdidas:** si una ventana se completa y el `main` todavía no consumió la anterior, se descarta y se incrementa `ventanas_perdidas`. Pasa cuando la UART es más lenta que la adquisición (fs altas o N grande).
- **Reinicio:** al cambiar de modo o de frecuencia, `reiniciar_adquisicion()` descarta la ventana en curso para no mezclar muestras tomadas con distinta fs.
- **Salida de Bypass:** buffer circular `buffer_out` de 512 muestras, escrito por la ISR del ADC y leído por la ISR del timer.

### Cálculo de la FFT

1. `arm_copy_q15`: copia la ventana, porque `arm_rfft_q15` destruye su entrada.
2. `arm_rfft_q15`: FFT real de N puntos. Entrega `2·N` valores (parte real e imaginaria intercaladas).
3. `arm_cmplx_mag_q15`: módulo de los primeros `N/2` bins (de 0 Hz a `fs/2`).

Cada bin `k` corresponde a la frecuencia:

```
f(k) = k · fs / N
```

| Parámetro | Fórmula |
|---|---|
| Resolución en frecuencia | `fs / N` |
| Frecuencia máxima | `fs / 2` |
| Duración de la ventana | `N / fs` |

## Protocolo serie

Una línea de texto por ventana, terminada en `\r\n`:

```
FFT,<fs_Hz>,<N>,<mag0>,<mag1>,...,<mag(N/2-1)>
```

- `fs_Hz`: frecuencia de muestreo real en Hz.
- `N`: tamaño de la ventana.
- `mag0 … mag(N/2-1)`: magnitud de cada bin (enteros con signo, formato Q15 crudo).

El baudrate es el configurado para la consola de debug (por defecto 115200; verificar `BOARD_DEBUG_UART_BAUDRATE`).

## Compilación y carga

1. Importar el proyecto en **MCUXpresso IDE**.
2. Elegir el tamaño de ventana en `main.c`:
   ```c
   #define FFT_SIZE   512U     // 512, 1024 o 2048
   ```
3. Verificar que la librería **CMSIS-DSP** esté incluida en el proyecto y que las tablas del tamaño elegido estén habilitadas. Si no lo están, `arm_rfft_init_q15` devuelve error y el programa se detiene mostrando un mensaje.
4. Compilar y cargar en la placa.
5. Para ver mensajes de depuración, descomentar `#define MODO_DEBUG 1`. No usarlo con fs altas: `PRINTF` es lento y se mezcla con las tramas del espectro.

## Visualización con Octave

Requisitos:

- Octave 7 o superior.
- Paquete `instrument-control`:
  ```
  pkg install -forge instrument-control
  ```
  En Debian/Ubuntu también se puede instalar con `sudo apt install octave-instrument-control`.

Uso:

1. Conectar la placa y averiguar el puerto:
   - Linux: `ls /dev/ttyACM*`
   - Windows: Administrador de dispositivos → Puertos (COM y LPT)
2. Editar la configuración al inicio de `fft_monitor.m`:
   ```octave
   PUERTO    = "/dev/ttyACM0";   % o "COM3" en Windows
   BAUDRATE  = 115200;
   USAR_DB   = false;            % true = eje Y en dB
   QUITAR_DC = true;             % ignora el bin 0
   ```
3. Cerrar cualquier terminal serie que tenga el puerto abierto.
4. Ejecutar el script, pasar la placa a modo FFT con SW3 y cambiar la frecuencia con SW2.

El título del gráfico muestra `fs`, `N`, la resolución, la frecuencia del pico y la cantidad de ventanas recibidas. En Linux, si da error de permisos, agregar el usuario al grupo `dialout` (`sudo usermod -aG dialout $USER`) y reiniciar la sesión.

## Pruebas sugeridas

1. **Frecuencia conocida:** aplicar una senoidal de 1 kHz. El pico debe aparecer en 1 kHz con un error menor a `fs/N`.
2. **Cambio de fs:** con SW2, el pico no debe moverse, pero cambian el rango del eje y la resolución.
3. **Aliasing:** aplicar una señal por encima de `fs/2` y observar dónde aparece el pico.
4. **Tamaño de ventana:** repetir con `FFT_SIZE` = 512, 1024 y 2048 y comparar resolución, tiempo de ventana y ventanas perdidas.

## Informe Del Trabajo Practico
-  https://docs.google.com/document/d/1UYwHyWoHy4OGFFGCWYdIYHNSK2_5kldYm8E4dJ9gnzo/edit?usp=sharing
