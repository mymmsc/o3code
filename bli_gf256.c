#include <stdlib.h>
#include <stdio.h>

#include "netcode.h"
#include "bli_gf256.h"
#if !defined(__API_BSD__) || defined(i386)
#  define __SSE3__
#  define __SSE2__
#  define __SSE__
#  define __MMX__
#  include <emmintrin.h>
//#  include <pmmintrin.h>
#endif
//////////////////////////////////////////////////////////////////////////////////////////

struct gf256_t 
{
    api_byte_t inverse_table[256];
    api_int32_t *b2j;
    api_int32_t *j2b;
};

//////////////////////////////////////////////////////////////////////////////////////////

static const char PrimPoly8 = 0x1d;	/* x^8 + x^4 + x^3 + x^2 + 1 */

#if !defined(__API_BSD__) || defined(i386)
#else

const vector api_byte_t PrimPoly_128 = (vector api_byte_t)
        (PrimPoly8, PrimPoly8, PrimPoly8, PrimPoly8,
         PrimPoly8, PrimPoly8, PrimPoly8, PrimPoly8,
         PrimPoly8, PrimPoly8, PrimPoly8, PrimPoly8,
         PrimPoly8, PrimPoly8, PrimPoly8, PrimPoly8);
#endif
//////////////////////////////////////////////////////////////////////////////////////////

// caller should have checked before to make sure the number is non-zero...
static api_uint32_t find_least_sig_set_bit(api_uint32_t number)
{
    api_uint32_t position;
    
#ifdef __API_WIN32__
    __asm
    {
        bsf  eax, number;	// bsfl: 0F BC
        mov  position, eax;
    }
#else	// not windows
#  if !defined(POWER_PC)
     asm ("bsfl %1, %0" : "=r" (position) : "rm" (number));
#  else		// POWER_PC
     assert(false) ;
     // for PPC, it seems taht cntl counts from most sig bit.. check further...
     asm ("{cntlz|cntlzw} %1, %0" : "=r" (position) : "r" (number));
#  endif		// POWER_PC
#endif	// not Windows
    
    return position;
}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructing gf256 object instances.
 */
API_DECLARE(api_status_t) gf256_create(gf256_t **gf)
{
    api_status_t rv = API_SUCCESS;
    // filling up the inverse table
    api_int32_t i, j;
    (*gf) = calloc(1, sizeof(gf256_t));
    // initializing the base tabled-based gf
    gf256_base_initialize(*gf);
    
    for (i = 0; i < 256; i ++)
    {
        for (j = 0; j < 256; j ++)
        {
            if (gf256_base_multiply((*gf), i, j) == 1)
            {
                break;
            }
        }
        (*gf)->inverse_table[i] = (j < 256) ? j : 0;
    }
    
    return rv;
}

API_DECLARE(api_status_t) gf256_destory(gf256_t **gf)
{
    // uninitializing base GF
    gf256_base_uninitialize(*gf);
    free(*gf);
    (*gf) = NULL;
    return API_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Multiplying a and b in gf256 using the loop-based method.
 */
API_DECLARE(api_byte_t) gf256_multiply(gf256_t *gf, api_byte_t a, api_byte_t b)
{
    api_byte_t hi_bit_set;
    api_byte_t result;
    api_int32_t i;
    result = 0;
    for (i = 0; i < 8; i ++)
    {
        if ((b & 1) !=  0)
        {
            result ^= a;
        }
        hi_bit_set = (a & 0x80) != 0;
        a <<= 1;
        
        if (hi_bit_set == 1)
        {
            a ^= PrimPoly8; /* x^8 + x^4 + x^3 + x + 1 */
        }
        b >>= 1;
        if (b == 0)
        {
            break;
        }
    }
    return result;
}

/**
 * Dividing a and b in gf256 using the loop-based method.
 */
API_DECLARE(api_byte_t) gf256_divide(gf256_t *gf, api_byte_t a, api_byte_t b)
{
    return gf256_multiply(gf, a, gf->inverse_table[b]);
}

/**
 * Multiplying an array of gf256 values by a factor and
 * writing the result in another array. The result is xor-ed with
 * the previous content of the output array.
 */
API_DECLARE(void) gf256_multiply_array(gf256_t *gf, api_byte_t * pBufSrc, api_byte_t * pBufDest,
                           api_byte_t in_factor, api_int32_t nSizeHexWords)
{
    api_byte_t factor;
    api_int32_t i;
    // an input 0 factor wouldn't change the output here
    if ( in_factor == 0 )
    {
		return;
    }
    // SSE2 code
#if !defined(__API_BSD__) || defined(i386)
    {
    __m128i hi_bits_mask_128, PrimPolyMask;
    __m128i leftOp, result;
    __m128i* pBufSrcHex = (__m128i*)pBufSrc;
    __m128i* pBufDestHex = (__m128i*)pBufDest;
    __m128i PrimPoly_128 = _mm_set1_epi8(PrimPoly8);
    __m128i zero_128 = _mm_setzero_si128();
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        leftOp = _mm_load_si128(pBufSrcHex);  
        factor = in_factor;        
        result = _mm_setzero_si128();        
        
        while ( factor != 0 )
		{
            if ((factor & 1) !=  0)
            {
                result = _mm_xor_si128(result, leftOp);
            }
            // Using signed comparision with 0 to build 0xff/0x00 pattern
            // for high bit set/clear
            hi_bits_mask_128 = _mm_cmplt_epi8(leftOp, zero_128);
            
            // Shift left of leftOp (<<= 1)
            leftOp = _mm_add_epi8(leftOp, leftOp);
            
            // Update of leftOp with primitive poly pattern
            PrimPolyMask = _mm_and_si128(hi_bits_mask_128, PrimPoly_128);
            leftOp = _mm_xor_si128(leftOp, PrimPolyMask);
            
            factor >>= 1;
        }
        
        *pBufDestHex = _mm_xor_si128(*pBufDestHex, result);
        pBufSrcHex ++;
        pBufDestHex ++;
    }
    }
#else	// PowerPC
    vector api_byte_t hi_bits_mask_128, PrimPolyMask;
    vector api_byte_t leftOp, result;
    vector api_byte_t* pBufSrcHex = (vector api_byte_t*)pBufSrc;
    vector api_byte_t* pBufDestHex = (vector api_byte_t*)pBufDest;
    vector char zero_128 = vec_xor(zero_128, zero_128);
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        leftOp = *pBufSrcHex;
        factor = in_factor;
        
        result = vec_xor(result, result);
        
        while ( factor != 0 )
        {
            if ((factor & 1) !=  0)
            {
                result = vec_xor(result, leftOp);
            }
            // Using signed comparision with 0 to build 0xff/0x00 pattern
            // for high bit set/clear
            hi_bits_mask_128 = vec_cmplt((vector char)leftOp, zero_128);
            
            // Shift left of leftOp (<<= 1)
            leftOp = vec_add(leftOp, leftOp);
            
            // Update of leftOp with primitive poly pattern
            PrimPolyMask = vec_and(hi_bits_mask_128, PrimPoly_128);
            leftOp = vec_xor(leftOp, PrimPolyMask);
            
            factor >>= 1;
        }
        
        *pBufDestHex = vec_xor(*pBufDestHex, result);
        pBufSrcHex ++;
        pBufDestHex ++;
    }
#endif
}

/**
 * Multiplying an array of gf256 values by a factor and
 * writing the result in another array. The previous content of
 * the output array is overwritten.
 */
API_DECLARE(void) gf256_multiply_array_write(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                                 api_byte_t in_factor, api_int32_t nSizeHexWords)
{
    api_byte_t factor;
    api_int32_t i;
    // SSE2 code
#if !defined(__API_BSD__) || defined(i386)
    
    __m128i hi_bits_mask_128, PrimPolyMask;
    __m128i leftOp, result;
    __m128i* pBufSrcHex = (__m128i*)pBufSrc;
    __m128i* pBufDestHex = (__m128i*)pBufDest;
    __m128i PrimPoly_128 = _mm_set1_epi8(PrimPoly8);
    __m128i zero_128 = _mm_setzero_si128();
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        leftOp = _mm_load_si128(pBufSrcHex);
        factor = in_factor;
        result = _mm_setzero_si128();
        
        while ( factor != 0 )
        {
            if ((factor & 1) !=  0)
            {
                result = _mm_xor_si128(result, leftOp);
            }
            // Using signed comparision with 0 to build 0xff/0x00 pattern
            // for high bit set/clear
            hi_bits_mask_128 = _mm_cmplt_epi8(leftOp, zero_128);
            
            // Shift left of leftOp (<<= 1)
            leftOp = _mm_add_epi8(leftOp, leftOp);
            
            // Update of leftOp with primitive poly pattern
            PrimPolyMask = _mm_and_si128(hi_bits_mask_128, PrimPoly_128);
            leftOp = _mm_xor_si128(leftOp, PrimPolyMask);
            
            factor >>= 1;
        }
        
        _mm_store_si128(pBufDestHex, result);
        pBufSrcHex ++;
        pBufDestHex ++;
    }
#else	// PowerPC
    vector api_byte_t hi_bits_mask_128, PrimPolyMask;
    vector api_byte_t leftOp, result;
    vector api_byte_t* pBufSrcHex = (vector api_byte_t*)pBufSrc;
    vector api_byte_t* pBufDestHex = (vector api_byte_t*)pBufDest;
    vector char zero_128 = vec_xor(zero_128, zero_128);
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        leftOp = *pBufSrcHex;        
        factor = in_factor;        
        result = vec_xor(result, result);        
        
		while ( factor != 0 )
        {
            if ((factor & 1) !=  0)
            {
                result = vec_xor(result, leftOp);
            }
            // Using signed comparision with 0 to build 0xff/0x00 pattern
            // for high bit set/clear
            hi_bits_mask_128 = vec_cmplt((vector char)leftOp, zero_128);
            
            // Shift left of leftOp (<<= 1)
            leftOp = vec_add(leftOp, leftOp);
            
            // Update of leftOp with primitive poly pattern
            PrimPolyMask = vec_and(hi_bits_mask_128, PrimPoly_128);
            leftOp = vec_xor(leftOp, PrimPolyMask);
            
            factor >>= 1;
        }
        
        *pBufDestHex = result;
        pBufSrcHex ++;
        pBufDestHex ++;
    }
#endif
}

/**
 * Multiplying an array of gf256 values by a factor and
 * writing the result in another array. The previous content of
 * the output array is overwritten.
 * It also returns the index ofthe first non-zero byte in the output array.
 */
API_DECLARE(api_int32_t) gf256_multiply_array_coeff(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                                 api_byte_t in_factor, api_int32_t nSizeHexWords)
{
    api_byte_t factor;
	api_int32_t index = -1, i;	// as if no non-zero found
	api_byte_t bNonZeroFound = 1;
	
    // SSE2 code
#if !defined(__API_BSD__) || defined(i386)
	const api_int32_t maskAllZero = 0x0000FFFF;
	api_int32_t mask16 ;
    
    __m128i hi_bits_mask_128, PrimPolyMask;
    __m128i leftOp, result;
    __m128i* pBufSrcHex = (__m128i*)pBufSrc;
    __m128i* pBufDestHex = (__m128i*)pBufDest;
    __m128i PrimPoly_128 = _mm_set1_epi8(PrimPoly8);
    __m128i zero_128 = _mm_setzero_si128();
    
    //__m128i factor_128 = _mm_set1_epi8(in_factor);
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        leftOp = _mm_load_si128(pBufSrcHex);
        
        factor = in_factor;
        
        result = _mm_setzero_si128();
        
        while ( factor != 0 )
        {
            if ((factor & 1) !=  0)
            {
                result = _mm_xor_si128(result, leftOp);
            }
            // Using signed comparision with 0 to build 0xff/0x00 pattern
            // for high bit set/clear
            hi_bits_mask_128 = _mm_cmplt_epi8(leftOp, zero_128);
            
            // Shift left of leftOp (<<= 1)
            leftOp = _mm_add_epi8(leftOp, leftOp);
            
            // Update of leftOp with primitive poly pattern
            PrimPolyMask = _mm_and_si128(hi_bits_mask_128, PrimPoly_128);
            leftOp = _mm_xor_si128(leftOp, PrimPolyMask);
            
            factor >>= 1;
        }
		
		if ( bNonZeroFound == 0)
        {
			result = _mm_xor_si128(*pBufDestHex, result);
			_mm_store_si128(pBufDestHex, result);			
			hi_bits_mask_128 = _mm_cmpeq_epi8(result, zero_128) ;	// compare all 16 bytes to zero
			mask16 = _mm_movemask_epi8(hi_bits_mask_128) ;		// into lower word: a 1 bit means the corresponding byte matched (i.e., was zero)
			if ( mask16 != maskAllZero )
            {
				mask16 = ~mask16;
				index = i << 4;
				index += find_least_sig_set_bit(mask16) ;
                
				bNonZeroFound = 1;
                
				// if the input factor is 0, nothing else to do after finding 
				// the first non-zero byte; exit the main loop
				if ( in_factor == 0 )
                {
                    break;
                }
			}
		}
		else
        {
			*pBufDestHex = _mm_xor_si128(*pBufDestHex, result);		
        }
        pBufSrcHex ++;
        pBufDestHex ++;
    }
#else	// PowerPC
    vector api_byte_t hi_bits_mask_128, PrimPolyMask;
    vector api_byte_t leftOp, result;
    vector api_byte_t* pBufSrcHex = (vector api_byte_t*)pBufSrc;
    vector api_byte_t* pBufDestHex = (vector api_byte_t*)pBufDest;
    vector char zero_128 = vec_xor(zero_128, zero_128);
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        leftOp = *pBufSrcHex;        
        factor = in_factor;
        
        result = vec_xor(result, result);
        
        while ( factor != 0 )
        {
            if ((factor & 1) !=  0)
            {
                result = vec_xor(result, leftOp);
            }
            // Using signed comparision with 0 to build 0xff/0x00 pattern
            // for high bit set/clear
            hi_bits_mask_128 = vec_cmplt((vector char)leftOp, zero_128);
            
            // Shift left of leftOp (<<= 1)
            leftOp = vec_add(leftOp, leftOp);
            
            // Update of leftOp with primitive poly pattern
            PrimPolyMask = vec_and(hi_bits_mask_128, PrimPoly_128);
            leftOp = vec_xor(leftOp, PrimPolyMask);
            
            factor >>= 1;
        }
        
		if ( bNonZeroFound == false )
        {
			result = vec_xor(*pBufDestHex, result) ;
			*pBufDestHex = result;
            // check if any non-zero
			if ( vec_all_eq(result, zero_128) != 1 )
            {
				api_byte_t* pBytesPtr = (api_byte_t*)&result ;
				for ( api_int32_t j = 0 ; j < 16 ; j ++ )
                {
					if ( pBytesPtr[j] != 0 )
                    {
						index = (i << 4) + j;
						break;
					}
				}
                
				// there is a non-zero detected by the above vec_all_eq
				bNonZeroFound = true;
				// if the input factor is 0, nothing else to do after finding 
				// the first non-zero byte; exit the main loop
				if ( in_factor == 0 )
                {
                    break;
                }
			}
		}
		else
        {
			*pBufDestHex = vec_xor(*pBufDestHex, result);
        }
        pBufSrcHex ++;
        pBufDestHex ++;
    }
#endif
	return index;
}

API_DECLARE(api_int32_t) gf256_find_non_zero_byte(gf256_t *gf, api_byte_t* pBuf, api_int32_t nSizeHexWords)
{
	api_int32_t index = -1, i;	// as if no non-zero found
    // SSE2 code
#if !defined(__API_BSD__) || defined(i386)
	const api_int32_t maskAllZero = 0x0000FFFF;
	api_int32_t mask16;
    __m128i hi_bits_mask_128;
    __m128i hex_word;
	__m128i* pBufHex = (__m128i*)pBuf;
    __m128i zero_128 = _mm_setzero_si128();
    
    for (i = 0; i < nSizeHexWords; i ++)
    {
        hex_word = _mm_load_si128(pBufHex);
        
		hi_bits_mask_128 = _mm_cmpeq_epi8(hex_word, zero_128) ;	// compare all 16 bytes to zero
		mask16 = _mm_movemask_epi8(hi_bits_mask_128) ;		// into lower word: a 1 bit means the corresponding byte matched (i.e., was zero)
		if ( mask16 != maskAllZero )
        {
			mask16 = ~mask16;
			index = i << 4;
			index += find_least_sig_set_bit(mask16);
			break;
		}
		pBufHex ++;
    }
#else	// PowerPC
    vector api_byte_t hex_word;
    vector api_byte_t* pBufHex = (vector api_byte_t*)pBuf;
    vector char zero_128 = vec_xor(zero_128, zero_128);
    
    for (api_int32_t i = 0; i < nSizeHexWords; i ++)
    {
		hex_word = *pBufHex;
        
		// check if any non-zero
		if ( vec_all_eq(hex_word, zero_128) != 1 )
        {
			api_byte_t* pBytesPtr = (api_byte_t*)pBufHex;
			for ( api_int32_t j = 0 ; j < 16 ; j ++ )
            {
				if ( pBytesPtr[j] != 0 )
                {
					index = (i << 4) + j;
					break;
				}
			}
			break;	// from outer loop
		}
        
        pBufHex ++;
    }
#endif
	return index;
}

API_DECLARE(api_byte_t) gf256_get_inverse(gf256_t *gf, api_byte_t a)
{
    return gf->inverse_table[a];
}

/**
 * Dividing an array of gf256 values by a factor and
 * writing the result in another array. The previous content of
 * the output array is overwritten.
 */
API_DECLARE(void) gf256_divide_array_write(gf256_t *gf, api_byte_t* pBufSrc, api_byte_t* pBufDest,
                                      api_byte_t factor, api_int32_t nSizeHexWords)
{
    gf256_divide_array_write(gf, pBufSrc, pBufDest, gf->inverse_table[factor], nSizeHexWords);
}

//////////////////////////////////////////////////////////////////////////////////////////

// Legacy GF methods

#define prim_poly_32 020000007
#define prim_poly_16 0210013
#define prim_poly_8  0435
#define prim_poly_4  023
#define prim_poly_2  07

static const api_int32_t Modar_nw   = 256;
static const api_int32_t Modar_nwm1 = 255;
static const api_int32_t Modar_poly = prim_poly_8;

API_DECLARE(void) gf256_base_initialize(gf256_t *gf)
{
    api_int32_t j, b;
    
    gf->b2j = (api_int32_t *) malloc(sizeof(api_int32_t)*Modar_nw);
    
    if (gf->b2j == NULL)
    {
        perror("gf_initialize, malloc gf->b2j");
    }
    /* When the word size is 8 bits, make three copies of the table so that
        you do not have to do the extra addition or subtraction in the
        multiplication/division routines */
    
    gf->j2b = (api_int32_t *) malloc(sizeof(api_int32_t) * Modar_nw * 3);
    
    if (gf->j2b == NULL)
    {
        perror("gf_initialize, malloc gf->j2b");
    }
    for (j = 0; j < Modar_nw; j++)
    {
        gf->b2j[j] = Modar_nwm1;
        gf->j2b[j] = 0;
    }
    
    b = 1;
    for (j = 0; j < Modar_nwm1; j++)
    {
        if (gf->b2j[b] != Modar_nwm1)
        {
            fprintf(stderr, "Error: j=%d, b=%d, B->J[b]=%d, J->B[j]=%d (0%o)\n",
                    j, b, gf->b2j[b], gf->j2b[j], (b << 1) ^ Modar_poly);
        }
        gf->b2j[b] = j;
        gf->j2b[j] = b;
        b = b << 1;
        if (b & Modar_nw)
        {
            b = (b ^ Modar_poly) & Modar_nwm1;
        }
    }
    
    for (j = 0; j < Modar_nwm1; j++)
    {
        gf->j2b[j + Modar_nwm1] = gf->j2b[j];
        gf->j2b[j + 2 * Modar_nwm1] = gf->j2b[j];
    }
    gf->j2b += Modar_nwm1;
}

// Release the allocated memory of the base GF table.
API_DECLARE(void) gf256_base_uninitialize(gf256_t *gf)
{
    free(gf->b2j);
    gf->b2j = NULL;
    // Restoring the pointer before releasing the memory.
    gf->j2b -= Modar_nwm1;
    free(gf->j2b);
    gf->j2b = NULL;
}

API_DECLARE(api_int32_t) gf256_base_multiply(gf256_t *gf, api_int32_t x, api_int32_t y)
{
    if (x == 0 || y == 0)
    {
        return 0;
    }
    return gf->j2b[gf->b2j[x] + gf->b2j[y]];
}

API_DECLARE(api_int32_t) gf256_base_divide(gf256_t *gf, api_int32_t a, api_int32_t b)
{
    api_int32_t sum_j;
    
    if (b == 0)
    {
        return -1;
    }
    if (a == 0)
    {
        return 0;
    }
    sum_j = gf->b2j[a] - gf->b2j[b];
    return (api_int32_t) gf->j2b[sum_j];
}

//////////////////////////////////////////////////////////////////////////////////////////
