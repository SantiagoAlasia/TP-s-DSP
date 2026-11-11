/**
 * @file    fir_multitasa.c
 * @brief   Implementacion muestra a muestra de los filtros multitasa.
 *
 *   PB     : FIR directo a fs.
 *   PA, EB : y[n] = x[n-D] - v[n],   v = camino de banda angosta:
 *               x --[HD]--(M:1)--[G]--(1:M)--[M*HD]--> v
 *   PBanda : PA seguido de un FIR directo (LPB).
 *
 * Misma implementacion que TP2.c:
 *   - Los FIR (PB, LPB, G) se calculan con fir_muestra(): historia duplicada
 *     (la ventana es contigua y no se copia nada) y __SMLALD con acumulador de
 *     64 bits. No se usa arm_fir_q15() con blockSize = 1, que copia el estado
 *     entero en cada llamada (~10 ciclos por coeficiente).
 *   - HD repartido: cada x[n] suma sus L = HD_PAD / M productos a los L
 *     diezmados pendientes, en lugar de calcular los NUM_TAPS_HD productos de
 *     golpe en la muestra de diezmado. Da el mismo u[k], sin el pico.
 *
 * La API (fir_multitasa.h) no cambia.
 */
#include <string.h>
#include "fir_multitasa.h"
#include "coef_multitasa.h"

#define MAX_TAPS_G ((NUM_TAPS_G_PA) > (NUM_TAPS_G_EB) ? (NUM_TAPS_G_PA) : (NUM_TAPS_G_EB))

/* Lectura de dos q15 como un word. No se usa read_q15x2() de CMSIS: hace
 * memcpy() y el proyecto compila con -fno-builtin, asi que es una llamada
 * real por lectura. Con packed el compilador emite un LDR, que en el M33
 * acepta direcciones no alineadas (la ventana puede empezar en un q15 impar). */
typedef struct { int32_t v; } __attribute__((packed)) word_noalineado_t;
#define LEER_Q15X2_IA(p) \
    (((p) += 2), ((const word_noalineado_t *)((p) - 2))->v)

/* FIR de una muestra: coeficientes en orden CMSIS (el primero multiplica a la
 * muestra mas vieja) e historia duplicada hist[p] / hist[p + n]. */
typedef struct {
    const q15_t *coef;
    uint16_t     n;        /* numTaps */
    uint16_t     pos;
    q15_t       *hist;     /* 2 * n muestras */
} fir_muestra_t;

/* ---- Configuracion del filtro activo ---- */
static volatile uint8_t s_activo = 0U;      /* 0 mientras se reconfigura -> bypass */
static uint8_t   s_angosto;                  /* 1 = PA / PBanda / EB (camino angosto) */
static uint8_t   s_tiene_post;               /* FIR directo a fs: PB (solo) o LPB (PBanda) */
static uint16_t  s_M;                        /* factor de diezmado, fs / F_BAJA_HZ */
static uint16_t  s_L;                        /* coeficientes de HD por fase, HD_PAD / M */
static uint16_t  s_D;                        /* retardo del complemento x[n - D] */
static const q15_t *s_hd;                    /* HD rellenado con ceros hasta L * M */
static uint32_t  s_retardo;

/* ---- Estados ---- */
static fir_muestra_t s_post;                 /* PB o LPB a fs */
static fir_muestra_t s_g;                    /* G_PA o G_EB a F_BAJA_HZ */
static q15_t   s_hist_post[2U * MAX_TAPS_FS];
static q15_t   s_hist_g[2U * MAX_TAPS_G];
static q15_t   s_hist_x[LONG_HIST_X];        /* historia de x, para x[n - D] */
static uint16_t s_pos_x;                     /* posicion de x[n] en s_hist_x */
static int64_t s_acc_hd[MAX_HIST_W];         /* diezmados pendientes (HD repartido) */
static q15_t   s_hist_w[MAX_HIST_W];         /* salidas de G, s_hist_w[0] la mas nueva */
static uint16_t s_fase;                      /* n % M */
static uint16_t s_ranura;                    /* (n / M) % L: acumulador de u[n / M] */

static q15_t fir_muestra(fir_muestra_t *f, q15_t x)
{
    const uint32_t n = f->n;
    uint32_t p = f->pos;

    f->hist[p]     = x;
    f->hist[p + n] = x;
    p = (p + 1U == n) ? 0U : p + 1U;
    f->pos = (uint16_t)p;

    const q15_t *px = &f->hist[p];          /* de la mas vieja a la mas nueva */
    const q15_t *pb = f->coef;
    int64_t acc = 0;

    for (uint32_t k = n >> 2; k > 0U; k--) {
        int32_t x0 = LEER_Q15X2_IA(px);
        int32_t c0 = LEER_Q15X2_IA(pb);
        int32_t x1 = LEER_Q15X2_IA(px);
        int32_t c1 = LEER_Q15X2_IA(pb);
        acc = __SMLALD(x0, c0, acc);
        acc = __SMLALD(x1, c1, acc);
    }
    if (n & 2U) {
        int32_t x0 = LEER_Q15X2_IA(px);
        int32_t c0 = LEER_Q15X2_IA(pb);
        acc = __SMLALD(x0, c0, acc);
    }
    if (n & 1U) {
        acc += (int32_t)*px * *pb;
    }
    return (q15_t)__SSAT((int32_t)(acc >> 15), 16);
}

/* Una muestra del camino angosto. Devuelve x[n - D] - v[n] (PA o EB).
 * Con fase p = n % M, x[n] = x[kM - j] aporta a u[n/M] con j = 0 si p == 0, y
 * a u[n/M + 1], u[n/M + 2]... con j = M - p, 2M - p... */
static q15_t complemento(q15_t x)
{
    const uint16_t M = s_M;
    const uint16_t L = s_L;

    s_pos_x = (uint16_t)((s_pos_x + 1U == LONG_HIST_X) ? 0U : s_pos_x + 1U);
    s_hist_x[s_pos_x] = x;

    uint16_t j = (s_fase == 0U) ? 0U : (uint16_t)(M - s_fase);
    uint16_t r = (s_fase == 0U) ? s_ranura
                                : (uint16_t)((s_ranura + 1U == L) ? 0U : s_ranura + 1U);
    for (uint16_t i = 0U; i < L; i++) {
        s_acc_hd[r] += (int32_t)s_hd[j] * x;
        j = (uint16_t)(j + M);
        r = (uint16_t)((r + 1U == L) ? 0U : r + 1U);
    }

    /* Instante de diezmado: u[n/M] esta completo y pasa por G a F_BAJA_HZ */
    if (s_fase == 0U) {
        q15_t u = (q15_t)__SSAT((int32_t)(s_acc_hd[s_ranura] >> 15), 16);
        q15_t w;
        s_acc_hd[s_ranura] = 0;
        w = fir_muestra(&s_g, u);
        for (uint16_t m = (uint16_t)(L - 1U); m > 0U; m--) {
            s_hist_w[m] = s_hist_w[m - 1U];
        }
        s_hist_w[0] = w;
    }

    /* Interpolador polifasico: v[n] = M * sum_m HD[p + mM] w[n/M - m] */
    int64_t acc = 0;
    const q15_t *h = &s_hd[s_fase];
    for (uint16_t m = 0U; m < L; m++) {
        acc += (int32_t)h[(uint32_t)m * M] * s_hist_w[m];
    }
    int32_t v = __SSAT((int32_t)((acc * M) >> 15), 16);

    if (++s_fase == M) {
        s_fase = 0U;
        s_ranura = (uint16_t)((s_ranura + 1U == L) ? 0U : s_ranura + 1U);
    }

    int32_t k = (int32_t)s_pos_x - (int32_t)s_D;
    if (k < 0) {
        k += (int32_t)LONG_HIST_X;
    }
    return (q15_t)__SSAT((int32_t)s_hist_x[k] - v, 16);
}

int mt_init(uint8_t idx_fs, mt_tipo_t tipo)
{
    const q15_t *coef_g = 0;
    uint16_t     taps_g = 0U;
    const q15_t *coef_post = 0;
    uint16_t     taps_post = 0U;

    s_activo = 0U;

    if ((idx_fs >= NUM_FS) || ((unsigned)tipo > (unsigned)MT_EB)) { return -1; }

    s_M       = m_tabla[idx_fs];
    s_hd      = hd_tabla[idx_fs];
    s_L       = (uint16_t)(hd_pad[idx_fs] / s_M);
    s_D       = 0U;
    s_angosto = 0U;  s_tiene_post = 0U;

    if (s_L > MAX_HIST_W) { return -1; }

    switch (tipo)
    {
        case MT_PB:
            coef_post = pb_tabla[idx_fs];   taps_post = pb_taps[idx_fs];
            break;
        case MT_PA:
            s_angosto = 1U;  coef_g = G_PA;  taps_g = NUM_TAPS_G_PA;  s_D = d_pa_tabla[idx_fs];
            break;
        case MT_PBANDA:
            s_angosto = 1U;  coef_g = G_PA;  taps_g = NUM_TAPS_G_PA;  s_D = d_pa_tabla[idx_fs];
            coef_post = lpb_tabla[idx_fs];  taps_post = lpb_taps[idx_fs];
            break;
        case MT_EB:
            s_angosto = 1U;  coef_g = G_EB;  taps_g = NUM_TAPS_G_EB;  s_D = d_eb_tabla[idx_fs];
            break;
        default:
            return -1;
    }

    /* El script entrega los arreglos con un 0 al principio (numTaps par, sin
     * agregar retardo): se usan tal cual. */
    if (coef_post != 0)
    {
        if (taps_post > MAX_TAPS_FS) { return -2; }
        s_post.coef = coef_post;  s_post.n = taps_post;
        s_post.pos  = 0U;         s_post.hist = s_hist_post;
        memset(s_hist_post, 0, sizeof(s_hist_post));
        s_tiene_post = 1U;
    }
    if (coef_g != 0)
    {
        if (taps_g > MAX_TAPS_G) { return -3; }
        s_g.coef = coef_g;  s_g.n = taps_g;
        s_g.pos  = 0U;      s_g.hist = s_hist_g;
        memset(s_hist_g, 0, sizeof(s_hist_g));
    }
    if (s_angosto && (s_D >= LONG_HIST_X)) { return -4; }

    memset(s_hist_x, 0, sizeof(s_hist_x));
    memset(s_acc_hd, 0, sizeof(s_acc_hd));
    memset(s_hist_w, 0, sizeof(s_hist_w));
    s_pos_x  = 0U;
    s_fase   = 0U;
    s_ranura = 0U;

    /* Retardo de grupo informativo: PB = (taps-1)/2; PA/EB = D; PBanda = D + (taps_lpb-1)/2 */
    if (s_angosto == 0U)      { s_retardo = (taps_post > 0U) ? (uint32_t)((taps_post - 1U) / 2U) : 0U; }
    else if (s_tiene_post)    { s_retardo = (uint32_t)s_D + (taps_post - 1U) / 2U; }
    else                      { s_retardo = s_D; }

    s_activo = 1U;
    return 0;
}

q15_t mt_procesar(q15_t x)
{
    if (s_activo == 0U) { return x; }

    if (s_angosto == 0U)
    {
        /* PB: FIR directo a fs */
        return fir_muestra(&s_post, x);
    }

    x = complemento(x);                           /* PA o EB */
    if (s_tiene_post)
    {
        x = fir_muestra(&s_post, x);              /* PBanda: PA y despues LPB */
    }
    return x;
}

uint32_t mt_retardo_muestras(void)
{
    return s_retardo;
}
