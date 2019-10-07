#ifndef O3CODE_BLI_CODE_H
#define O3CODE_BLI_CODE_H
//////////////////////////////////////////////////////////////////////////////////////////

#include "netcode.h"
#include "bli_random.h"
#include "bli_gf256.h"
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif
//////////////////////////////////////////////////////////////////////////////////////////

#if NETCODE_USE_BLI

#define BLI_LINEAR_DEPENDENT ((xint_t **)(-1))

typedef enum {
	BLI_BASE   = 0X01, /**< base codec */
	BLI_FAST   = 0X02, /**< fast codec */
    BLI_THREAD = 0X80  /**< codec with thread */
}bli_mode_e;

#define BLI_IS_BASE(m)   ((m) & BLI_BASE)
#define BLI_IS_FAST(m)   ((m) & BLI_FAST)
#define BLI_IS_THREAD(m) ((m) & BLI_THREAD)

#define BLI_MODE BLI_BASE

typedef struct bli_netcode_t 
{
    int            density;       /**< The sparcity/density of the encoding coefficients */
    int            size;          /**< The size of the data block in terms of the number of bytes */
    int            num;           /**< The number of blocks in the original message */
//#if BLI_IS_FAST(BLI_MODE) > 0
#if BLI_MODE >= BLI_FAST
    int            sizeHex;       /**< The size of the data block in hex words. */
    int            nHex, remHex;  /**< The number of blocks in hex words unit, and the remainder. */
#endif
    int            lastRow;       /**< The number of encoded and decoded blocks so far received */
	bli_mode_e     coding_mode;   /**< The bli_code deployment mode */
    xint_t       **coef;          /**< The coefficients */
    xint_t       **feedback;      /**< The decoded blocks so far */
    xint_t        *keep_coef;     /**< Array for keeping coefficients for encode process */
    uint32_t  *colNonZero;    /**< Records the column index of the leading non-zero coefficient in each row */
	api_status_t  *memory_status; /**< Tracks whether each individual coef[i] is allocated by bli_code
                                      library or its memory area provided by the user in embedded coef scenario. */
    bli_random_t  *prEnc;         /**< The pseudo-random number generator for the encoder */
    bli_random_t  *prDec;         /**< The pseudo-random number generator for the decoder */
    gf256_t       *gf;            /**< The instance representing a Galois Field GF(2_n, not only 256 or 32 bits). */
}bli_netcode_t;

#endif
//////////////////////////////////////////////////////////////////////////////////////////

API_DECLARE_NONSTD(void *) bli_alloc(size_t s);
API_DECLARE_NONSTD(void *) bli_realloc(void *p, size_t s);
API_DECLARE_NONSTD(void)   bli_free(void *p);

//////////////////////////////////////////////////////////////////////////////////////////
API_DECLARE(api_status_t) bli_base_init(netcode_t *codec);
API_DECLARE_NONSTD(api_status_t) bli_packet_add(netcode_t *codec, xint_t * packet);
API_DECLARE(api_status_t) bli_isdecoded(bli_netcode_t *base);
API_DECLARE(uint32_t) bli_coef_make(bli_netcode_t *base);
API_DECLARE_NONSTD(xint_t **) bli_base_decode(netcode_t *codec, uint32_t seed, xint_t * encoded);
#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* ! O3CODE_BLI_CODE_H */
