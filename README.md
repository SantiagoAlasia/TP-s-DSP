# TP1 - Muestreo con MCXN947

Trabajo práctico de la materia **Procesamiento Digital de Señales**. Implementa un sistema de adquisición y reproducción de señal utilizando el conversor A/D y D/A integrados en el microcontrolador **MCXN947** (placa FRDM-MCXN947).

## Objetivo

Manejar el módulo ADC del MCU MCXN947 para digitalizar una señal analógica a distintas velocidades de muestreo, almacenarla en un buffer circular en formato de punto fijo Q15, y reproducirla a través del DAC integrado.

## Funcionalidades

- **Adquisición configurable:** el ADC digitaliza la señal de entrada a 5 velocidades de muestreo seleccionables: **8K, 16K, 22K, 44K y 48K muestras/segundo**.
- **Selección de frecuencia por botón (SW2):** cicla circularmente entre las 5 velocidades disponibles.
- **Run/Stop (SW3):** habilita o detiene la adquisición y reproducción sin perder el estado de frecuencia seleccionado.
- **Indicación visual:** un LED RGB indica la velocidad de muestreo activa mediante un código de colores.
- **Buffer circular:** las muestras se almacenan en un buffer de 512 posiciones en formato **Q15** (Q1.15, punto fijo con signo).
- **Reproducción por DAC:** las muestras adquiridas se reconvierten a 12 bits y se envían al DAC para reconstruir la señal.
- **Disparo sincronizado por hardware (CTIMER):** el CTIMER0 genera los eventos periódicos de match que disparan, por software, tanto la conversión del ADC como el envío de la siguiente muestra al DAC — garantizando un período de muestreo estable ($T_s$ constante).

## Máquina de estados

| Estado | ADC | Frecuencia | LED |
|:---:|:---:|:---:|:---:|
| 0 | OFF | — | Apagado |
| 1 | ON | 8 kS/s | Rojo |
| 2 | ON | 16 kS/s | Verde |
| 3 | ON | 22 kS/s | Azul |
| 4 | ON | 44 kS/s | Rojo + Verde |
| 5 | ON | 48 kS/s | Verde + Azul |

## Arquitectura

```
SW3 (Run/Stop) ─┐
                 ├─► GPIO IRQ ─► Máquina de Estados ─► LED RGB
SW2 (Frecuencia)─┘                      │
                                         ▼
                              CTIMER0 (Match 3)
                                         │
                        ┌────────────────┴────────────────┐
                        ▼                                  ▼
                 Trigger SW → ADC1                   Lectura buffer → DAC0
                        │
                        ▼
              Buffer circular Q15 (512 muestras)
```

- **CTIMER0 (Match 3):** genera la base de tiempo. Su `matchValue` se recalcula dinámicamente según el estado activo, para lograr cada una de las 5 frecuencias de muestreo.
- **LPADC1:** dispara conversión por software en cada evento de match. El resultado se convierte a Q15 y se guarda en el buffer de escritura.
- **DAC0:** en el mismo evento de match, se lee la siguiente muestra pendiente del buffer y se envía al conversor D/A tras reescalar de Q15 a 12 bits.

## Conversión de formatos

- **ADC → Q15:** el resultado del ADC (sin signo, centrado en la mitad de escala) se recentra restando el offset correspondiente, quedando una señal con signo en el rango aproximado [-1, 1).
- **Q15 → DAC (12 bits):** el valor Q15 se reescala y desplaza para quedar dentro del rango sin signo de 12 bits [0, 4095], saturando en los extremos.

## Hardware utilizado

- Placa **FRDM-MCXN947**
- Entrada analógica: pin **P1_23 (ADC1_A23)**
- Botones de usuario de la placa (Run/Stop y cambio de frecuencia)
- LED RGB integrado

## Herramientas

- **MCUXpresso IDE** + **MCUXpresso SDK**
- **Config Tools** (Pins, Clocks, Peripherals) para la configuración gráfica de ADC1, CTIMER0 y DAC0
- **CMSIS-DSP** (`arm_math.h`) para el tipo de dato Q15

## Estructura del repositorio

```
├── source/
│   └── TP1.c              # Código principal de la aplicación
├── board/                  # Configuración de la placa (generado por Config Tools)
├── drivers/                 # Drivers del SDK utilizados
└── README.md
```

## Cómo compilar

1. Clonar este repositorio
2. Abrir MCUXpresso IDE → **File → Import → Existing Projects into Workspace**
3. Seleccionar la carpeta clonada
4. Compilar (**Build**) y ejecutar (**Debug/Run**) sobre la placa FRDM-MCXN947 conectada por USB
