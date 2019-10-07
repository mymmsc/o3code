#ifndef O3CODE_BLI_GF256_H
#define O3CODE_BLI_GF256_H
//////////////////////////////////////////////////////////////////////////////////////////

#include <api.h>
#include <api/errno.h>
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup crypto_o3random Random Operations - The pseudo-random number generator
 * @ingroup Crypto 
 * @{
 */

typedef struct gf256_t gf256_t;

API_DECLARE(api_status_t) gf256_create(gf256_t **gf);
API_DECLARE(api_status_t) gf256_destory(gf256_t **gf);

API_DECLARE(api_byte_t) gf256_multiply(gf256_t *gf, api_byte_t a, api_byte_t b);
API_DECLARE(api_byte_t) gf256_divide(gf256_t *gf, api_byte_t a, api_byte_t b);
API_DECLARE(void) gf256_multiply_array(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                        api_byte_t in_factor, api_int32_t nSizeHexWords);

API_DECLARE(void) gf256_multiply_array_write(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                              api_byte_t in_factor, api_int32_t nSizeHexWords);

API_DECLARE(void) gf256_divide_array_write(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                            api_byte_t factor, api_int32_t nSizeHexWords);

API_DECLARE(api_int32_t) gf256_multiply_array_coeff(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                                 api_byte_t in_factor, api_int32_t nSizeHexWords) ;
								 
API_DECLARE(api_int32_t) gf256_find_non_zero_byte(gf256_t *gf, api_byte_t* pBuf, api_int32_t nSizeHexWords) ;

API_DECLARE(api_byte_t) gf256_get_inverse(gf256_t *gf, api_byte_t a);

API_DECLARE(api_int32_t) gf256_base_multiply(gf256_t *gf, api_int32_t x, api_int32_t y);
API_DECLARE(api_int32_t) gf256_base_divide(gf256_t *gf, api_int32_t a, api_int32_t b);

API_DECLARE(void) gf256_base_initialize(gf256_t *gf);
API_DECLARE(void) gf256_base_uninitialize(gf256_t *gf);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ! O3CODE_BLI_GF256_H */
