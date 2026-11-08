/**
 * @file    fir_multitasa.c
 * @brief   Implementacion muestra a muestra de los filtros multitasa.
 *
 *   PB     : FIR directo a fs.
 *   PA, EB : y[n] = x[n-D] - v[n],   v = camino de banda angosta:
 *               x --[HD]--(M:1)--[G]--(1:M)--[M*HD]--> v
 *   PBanda : PA seguido de un FIR directo (LPB).
 */
#include "fir_multitasa.h"
#include "coef_multitasa.h"

#define G_STATE_MAX   (((NUM_TAPS_G_PA) > (NUM_TAPS_G_EB) ? (NUM_TAPS_G_PA) : (NUM_TAPS_G_EB)) + 4U)
#define POST_STATE_MAX ((MAX_TAPS_FS) + 4U)

/* ---- Configuracion del filtro activo ---- */
static volatile uint8_t s_activo = 0U;      /* 0 mientras se reconfigura -> bypass */
static uint8_t   s_angosto;                  /* 1 = PA / PBanda / EB (camino angosto) */
static uint8_t   s_tiene_g;
static uint8_t   s_tiene_post;               /* FIR directo a fs: PB (solo) o LPB (PBanda) */
static uint8_t   s_M;
static uint16_t  s_D;
static uint16_t  s_nw;                       /* HD_PAD / M: largo de la historia de w */
static uint16_t  s_hd_taps;
static const q15_t *s_hd;

/* ---- Estados ---- */
static arm_fir_instance_q15 s_g;
static arm_fir_instance_q15 s_post;
static q15_t s_g_state[G_STATE_MAX];
static q15_t s_post_state[POST_STATE_MAX];
static q15_t s_xb[2U * LONG_HIST_X];         /* historia de x, duplicada (ver mt_procesar) */
static q15_t s_wb[MAX_HIST_W];               /* historia de w a F_BAJA, s_wb[0] = la mas nueva */
static uint16_t s_ip;                        /* posicion de x[n] en s_xb */
static uint8_t  s_fase;                      /* n % M */
static uint32_t s_retardo;

static inline q15_t sat_q15(int64_t v)
{
    if (v >  32767) { return  32767; }
    if (v < -32768) { return -32768; }
    return (q15_t)v;
}

int mt_init(uint8_t idx_fs, mt_tipo_t tipo)
{
    const q15_t *coef_g = 0;
    uint16_t     taps_g = 0U;
    const q15_t *coef_post = 0;
    uint16_t     taps_post = 0U;
    uint32_t     i;

    s_activo = 0U;

    if ((idx_fs >= NUM_FS) || ((unsigned)tipo > (unsigned)MT_EB)) { return -1; }

    s_M       = m_tabla[idx_fs];
    s_hd      = hd_tabla[idx_fs];
    s_hd_taps = hd_taps[idx_fs];
    s_nw      = (uint16_t)(hd_pad[idx_fs] / s_M);
    s_D       = 0U;
    s_angosto = 0U;  s_tiene_g = 0U;  s_tiene_post = 0U;

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

    /* arm_fir_q15 exige numTaps par; el script ya entrega los arreglos con un 0 al principio */
    if (coef_post != 0)
    {
        if ((taps_post > POST_STATE_MAX) ||
            (arm_fir_init_q15(&s_post, taps_post, (q15_t *)coef_post, s_post_state, 1U) != ARM_MATH_SUCCESS))
        {
            return -2;
        }
        s_tiene_post = 1U;
    }
    if (coef_g != 0)
    {
        if ((taps_g > G_STATE_MAX) ||
            (arm_fir_init_q15(&s_g, taps_g, (q15_t *)coef_g, s_g_state, 1U) != ARM_MATH_SUCCESS))
        {
            return -3;
        }
        s_tiene_g = 1U;
    }

    for (i = 0U; i < (2U * LONG_HIST_X); i++) { s_xb[i] = 0; }
    for (i = 0U; i < MAX_HIST_W; i++)         { s_wb[i] = 0; }
    s_ip   = 0U;
    s_fase = 0U;

    /* Retardo de grupo informativo: PB = (taps-1)/2; PA/EB = D; PBanda = D + (taps_lpb-1)/2 */
    if (s_angosto == 0U)      { s_retardo = (taps_post > 0U) ? (uint32_t)((taps_post - 1U) / 2U) : 0U; }
    else if (s_tiene_post)    { s_retardo = (uint32_t)s_D + (taps_post - 1U) / 2U; }
    else                      { s_retardo = s_D; }

    s_activo = 1U;
    return 0;
}

q15_t mt_procesar(q15_t x)
{
    q15_t y;
    int64_t acc;
    uint32_t m;

    if (s_activo == 0U) { return x; }

    if (s_angosto == 0U)
    {
        /* PB: FIR directo a fs */
        arm_fir_q15(&s_post, &x, &y, 1U);
        return y;
    }

    /* Historia de x: la muestra nueva va en la posicion MAS BAJA y se escribe dos veces
     * (ip e ip + LONG_HIST_X). Asi x[n-j] = s_xb[ip + j] para j = 0..LONG_HIST_X-1, contiguo,
     * y se puede usar arm_dot_prod_q15 sin indices modulares. */
    s_ip = (s_ip == 0U) ? (uint16_t)(LONG_HIST_X - 1U) : (uint16_t)(s_ip - 1U);
    s_xb[s_ip]                = x;
    s_xb[s_ip + LONG_HIST_X]  = x;

    /* Instante de diezmado (n % M == 0): HD a fs -> G a F_BAJA -> historia de w */
    if (s_fase == 0U)
    {
        q63_t acc63 = 0;
        q15_t u, w;
        arm_dot_prod_q15(s_hd, &s_xb[s_ip], s_hd_taps, &acc63);   /* Q30 */
        u = sat_q15(acc63 >> 15);
        arm_fir_q15(&s_g, &u, &w, 1U);
        for (m = (uint32_t)s_nw - 1U; m > 0U; m--) { s_wb[m] = s_wb[m - 1U]; }
        s_wb[0] = w;
    }

    /* Interpolador polifasico: v = M * sum_m HD[fase + m*M] * wb[m]   (64 bits: M*suma desborda 32) */
    acc = 0;
    for (m = 0U; m < s_nw; m++)
    {
        acc += (int32_t)s_hd[(uint32_t)s_fase + m * s_M] * (int32_t)s_wb[m];
    }
    acc = (acc * (int64_t)s_M) >> 15;

    /* Complemento: x[n-D] - v  (con saturacion) */
    y = sat_q15((int64_t)s_xb[s_ip + s_D] - acc);

    s_fase++;
    if (s_fase >= s_M) { s_fase = 0U; }

    if (s_tiene_post)
    {
        q15_t z;
        arm_fir_q15(&s_post, &y, &z, 1U);
        y = z;
    }
    return y;
}

uint32_t mt_retardo_muestras(void)
{
    return s_retardo;
}
