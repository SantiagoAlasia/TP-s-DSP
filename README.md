# TP4 – Filtros adaptativos con FRDM-MCXN947

Cátedra de DSP – Facultad de Ciencias Exactas, Físicas y Naturales – UNC

**Integrantes:**
- Feiguin, Lucia
- Monutti, Elena
- Alasia Santiago

Implementación de dos sistemas adaptativos (algoritmo LMS, 30 coeficientes, CMSIS-DSP) sobre la placa **FRDM-MCXN947**:

1. **Identificación de planta:** el filtro adaptativo aprende los coeficientes de una planta FIR conocida.
2. **Cancelación de ruido:** se recupera una senoidal externa a la que se le sumó ruido.

El control es por botones, los resultados de la parte 1 se envían por UART y se grafican en Octave, y la salida de la parte 2 se observa con un osciloscopio.

---

## Uso rápido

| Botón | Función |
|---|---|
| **SW2** | Cambia de etapa (ciclo: OFF → ID random → ID constante → Cancelación → OFF) |
| **SW3** | Pasa al siguiente valor de `mu` (vuelve al primero al llegar al último) |

Cada cambio de etapa o de `mu` **reinicia el filtro** (coeficientes en cero) y comienza un nuevo experimento.

| Etapa | LED | ADC / DAC | Qué hace |
|---|---|---|---|
| OFF | apagado | no se usan | No procesa |
| ID random | rojo | no se usan | Identificación con ruido gaussiano como entrada |
| ID constante | verde | no se usan | Identificación con entrada constante (0.5) |
| Cancelación | azul | se usan | Cancelación de ruido sobre la señal del ADC |

Valores de `mu` (los índices avanzan con SW3):

| Índice | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| Identificación (Q15) | 0.05 | 0.10 | 0.20 | 0.35 | 0.50 | 0.70 |
| Cancelación (float) | 0.001 | 0.002 | 0.005 | 0.01 | 0.02 | 0.05 |

---

## Cómo funciona

El **CTimer0** marca el período de muestreo y su interrupción dispara el procesamiento. La planta y el LMS de la parte 1 trabajan en formato **Q15**, muestra a muestra (`blockSize = 1`).

### Parte 1 – Identificación de planta

```
x (random o constante) ─┬─► PLANTA (FIR 30 coef) ─► d ──►(+)──► e = d − y
                        │                                ▲(−)
                        └─► FILTRO ADAPTATIVO (LMS) ─► y ┘
                                     ▲                          │
                                     └──────── actualiza ◄──────┘
```

- Cada **100 muestras** se calcula el **MSE** (promedio de `e²`) y la diferencia entre los coeficientes de la planta y los del adaptativo: `dw[k] = PLANTA_COEF[k] − w[k]`.
- Ese bloque se envía por UART (ver protocolo).
- Los coeficientes están guardados en el orden que usa CMSIS (invertido), por eso `dw` se calcula elemento a elemento.

### Parte 2 – Cancelación de ruido

```
random (ruido x) ─┬─► CORRELADOR (FIR 30 coef) ─►(+)◄── senoidal (ADC)
                  │                               │ d = señal + ruido correlacionado
                  │                               ▼
                  └─► FILTRO ADAPTATIVO (LMS) ─► y ─►(−)─► e = d − y ─► DAC ─► osciloscopio
```

- En cada tick del CTimer se **saca al DAC** la muestra calculada en el tick anterior y se **dispara el ADC**.
- La interrupción de fin de conversión pasa el resultado a Q15 y avisa al `main`, que calcula la nueva salida.
- El filtro adaptativo es **LMS en punto flotante**: `arm_lms_q15` trunca en cada actualización y eso introduce un sesgo que degrada la cancelación.
- La salida `e` converge a la senoidal limpia.

---

## Protocolo UART

Texto, una línea por mensaje, terminada en `\r\n`, a **115200 baudios**.

| Línea | Formato | Cuándo |
|---|---|---|
| Configuración | `C,seq,modo,mu_idx,mu_q15` | Al cambiar de etapa o de `mu` |
| Datos | `D,seq,mse_q30,dw0,…,dw29` | Cada 100 muestras (solo en identificación) |

- `seq`: número de experimento (sube en 1 con cada reinicio).
- `modo`: 0 = OFF, 1 = ID random, 2 = ID constante, 3 = Cancelación.
- `mse_q30`: MSE crudo. MSE real = `mse_q30 / 2^30`.
- `dw[k]`: diferencia de coeficientes en Q15. Valor real = `dw / 32768`.
- En cancelación solo se envía la línea `C`; la salida se mide en el osciloscopio.

---

## Archivos

| Archivo | Contenido |
|---|---|
| `main.c` | FSM, interrupciones (botones, CTimer, ADC), LMS, ADC/DAC, envío por UART |
| `fir_planta.h` | Coeficientes de la planta / correlador (30, Q15) e instancia del FIR |
| `fir_adaptativo.h` | Tablas de `mu`, coeficientes, estados e instancias de los LMS, variables del MSE |
| `uart_protocol.h` | Funciones `tx_cfg()` y `tx_data()` que dan formato a los mensajes |
| `semihost_hardfault.c` | Manejador de HardFault generado por MCUXpresso |
| `octave/recibir_simple.m` | Recibe los datos por el puerto serie y los grafica en tiempo real |

Opciones en `main.c`: `MODO_DEBUG` (mensajes de depuración) y `MODO_BYPASS` (el DAC saca la muestra cruda del ADC, útil para verificar la parte analógica).

---

## Cómo usarlo

**Requisitos:** MCUXpresso IDE con el SDK de la FRDM-MCXN947 (CMSIS-DSP incluido en el proyecto), y Octave con el paquete `instrument-control` (`pkg install -forge instrument-control`).

1. Importar el proyecto en MCUXpresso IDE, compilar y cargarlo con el depurador (cable USB al puerto MCU-Link).
2. **Parte 1:** cerrar cualquier terminal serie abierta, ejecutar `recibir_simple` en Octave (puerto `/dev/ttyACM0`, 115200; se cambia al inicio del script) y usar SW2/SW3 en la placa. Se grafican el MSE, la distancia entre coeficientes y `dw[k]` del último bloque.
3. **Parte 2:** conectar la señal del generador a la entrada del ADC y la salida del DAC al osciloscopio. La senoidal debe estar centrada en la mitad de la escala del ADC.
   - Entrada ADC: ADC0A0
   - Salida DAC: P4_2

---

## Resultados esperados

- **Entrada random:** el MSE baja varios órdenes de magnitud y la distancia entre coeficientes tiende a cero. El filtro identifica la planta.
- **Entrada constante:** el MSE también baja, pero la distancia entre coeficientes **se estanca**. Una señal constante solo excita la ganancia en continua, así que el filtro no puede distinguir entre infinitas combinaciones de coeficientes.
- **`mu` alto con entrada constante:** diverge si `mu > 2/(N·C²) ≈ 0.27` (N = 30, C = 0.5), por eso `mu = 0.35, 0.5, 0.7` no son estables en ese modo.
- **Cancelación:** al arrancar se ve la senoidal con ruido y, a medida que el filtro converge, la salida del DAC queda limpia.
