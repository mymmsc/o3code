#include "bli_code.h"
//////////////////////////////////////////////////////////////////////////////////////////

#define BLI_CODE_NUM(c)  (c->accelerator->num)
#define BLI_CODE_SIZE(c) (c->accelerator->size)

//////////////////////////////////////////////////////////////////////////////////////////
API_DECLARE(api_status_t) bli_accel_init(netcode_t *codec)
{
    api_status_t rv = API_SUCCESS;
    bli_netcode_t *accelerator = (bli_netcode_t *)codec->third_param;
    rv = bli_base_init(codec);
    assert((accelerator->size % 16) == 0);
    //assert((numBlocks % 16) == 0);
    accelerator->sizeHex = accelerator->size >> 4;
    accelerator->nHex	= accelerator->num >> 4;
	accelerator->remHex  = accelerator->num - (accelerator->nHex << 4);
    
    return rv;
}

API_DECLARE_NONSTD(api_status_t) bli_accel_destroy(netcode_t *codec)
{
    bli_netcode_t *accelerator = (bli_netcode_t *)codec->third_param;
    //bli_base_close(accelerator);
    return API_SUCCESS;
}

API_DECLARE_NONSTD(int) bli_accel_encode(netcode_t *codec, xint_t * outcome)
{
    int i;
    bli_netcode_t *accelerator = codec->third_param;
    int s = bli_coef_make(accelerator);
    
    // The first iteration is treated separately to write to the output
    // array rather than xor-ing with the previous content.
    
    gf256_multiply_array_write(accelerator->gf, accelerator->feedback[0], outcome, accelerator->keep_coef[0], accelerator->sizeHex);
    
    for (i = 1; i < accelerator->lastRow; i ++)
    {
        gf256_multiply_array(accelerator->gf, accelerator->feedback[i], outcome, accelerator->keep_coef[i], accelerator->sizeHex);
    }
    
    return s;
}

/**
 * Decode blocks in feedback.
 * @return decoded the decoded data blocks if decoding is successful. 
 */
static api_byte_t ** decode_blocks(bli_netcode_t *accelerator)
{
    int i, j, currentRow;
    api_byte_t factor;
    api_byte_t * t;
    api_byte_t inv_factor;
        
    // A. Reduce leading (and possibly trailing) coefficients in the
    // last row to zero.
    
    for (i = 0; i < accelerator->lastRow; i ++)
    {
        factor = accelerator->coef[accelerator->lastRow][accelerator->colNonZero[i]];
        
        gf256_multiply_array(accelerator->gf, accelerator->coef[i], accelerator->coef[accelerator->lastRow], factor, accelerator->nHex);
        gf256_multiply_array(accelerator->gf, accelerator->feedback[i], accelerator->feedback[accelerator->lastRow], factor, accelerator->sizeHex);
		
		for (j = accelerator->num - 1; j >= accelerator->num - accelerator->remHex; j --)
        {
            accelerator->coef[accelerator->lastRow][j] ^= gf256_base_multiply(accelerator->gf, accelerator->coef[i][j],
                                        factor);
        }
    }
    
    // B. Find the leading non-zero coefficient in the current row.
    factor = 0;
    
    for (i = 0; i < accelerator->num; i ++)
    {
        if (accelerator->coef[accelerator->lastRow][i] != 0)
        {
            factor = accelerator->coef[accelerator->lastRow][i];
            accelerator->colNonZero[accelerator->lastRow] = i;
            break;
        }
    }
    // C. Check for linear independence with rows above this one.
    if (factor == 0)
    {
        // a highly unlikely event if coefficients are randomly generated.
        return BLI_LINEAR_DEPENDENT;
    }
    
    inv_factor = gf256_get_inverse(accelerator->gf, factor);
    
    // D. Reduce the leading non-zero entry to 1, such that the result
    // conforms to the row echelon form (REF).
    {
        int div = accelerator->colNonZero[accelerator->lastRow] >> 4;
        int rem = accelerator->colNonZero[accelerator->lastRow] - (div << 4);
        int columnNonZeroHex = (rem == 0) ? div : div + 1;
        int columnNonZero_aligned = columnNonZeroHex << 4;
        
        // Process the coefficients till the next 16-th chunk,
        // in which case base multiply is faster than loop-based.
        
        for (i = accelerator->colNonZero[accelerator->lastRow]; i < columnNonZero_aligned; i ++)
        {
            accelerator->coef[accelerator->lastRow][i] = gf256_base_multiply(accelerator->gf, 
                accelerator->coef[accelerator->lastRow][i],
                inv_factor);
        }
        // Now do the remainder using accelerator SSE multiply.
        gf256_multiply_array_write(accelerator->gf, accelerator->coef[accelerator->lastRow] + columnNonZero_aligned, 
                                  accelerator->coef[accelerator->lastRow] + columnNonZero_aligned, 
                                  inv_factor, accelerator->nHex - columnNonZeroHex);
	    
	    if (accelerator->num - accelerator->remHex >= columnNonZero_aligned)
        {
            for (i = accelerator->num - 1; i >= accelerator->num - accelerator->remHex; i --)
            {
			    accelerator->coef[accelerator->lastRow][i] = gf256_base_multiply(accelerator->gf, 
                    accelerator->coef[accelerator->lastRow][i],
                    inv_factor);
            }
        }
        gf256_multiply_array_write(accelerator->gf, 
                                  accelerator->feedback[accelerator->lastRow], accelerator->feedback[accelerator->lastRow],
                                  inv_factor, accelerator->sizeHex);
        
        // E. Reduce the coefficient matrix to the reduced row-echelon form (RREF).
        
        // E.1. Reduce to zeros in entries above columnNonZero[lastRow].
        for (i = 0; i < accelerator->lastRow; i ++)
        {
            factor = accelerator->coef[i][accelerator->colNonZero[accelerator->lastRow]];		
		    gf256_multiply_array(accelerator->gf, accelerator->coef[accelerator->lastRow], accelerator->coef[i], factor, accelerator->nHex);
		    
		    for (j = accelerator->num - 1; j >= accelerator->num - accelerator->remHex; j --)
            {
                accelerator->coef[i][j] ^= gf256_base_multiply(accelerator->gf, 
                    accelerator->coef[accelerator->lastRow][j], factor);
            }
		    gf256_multiply_array(accelerator->gf, accelerator->feedback[accelerator->lastRow], accelerator->feedback[i], factor, accelerator->sizeHex);
        }
        
        // E.2. Reorder rows based on columnNonZero[].
        currentRow = accelerator->lastRow;
        while (currentRow > 0 && accelerator->colNonZero[currentRow] < accelerator->colNonZero[currentRow - 1])
        {
            int c;
            // Exchange two rows.
            t = accelerator->coef[currentRow - 1];
            accelerator->coef[currentRow - 1] = accelerator->coef[currentRow];
            accelerator->coef[currentRow] = t;
            
            t = accelerator->feedback[currentRow - 1];
            accelerator->feedback[currentRow - 1] = accelerator->feedback[currentRow];
            accelerator->feedback[currentRow] = t;
            
            c = accelerator->colNonZero[currentRow - 1];
            accelerator->colNonZero[currentRow - 1] = accelerator->colNonZero[currentRow];
            accelerator->colNonZero[currentRow] = c;
            
            currentRow --;
        }
        
        accelerator->lastRow ++;
    }
    return bli_isdecoded(accelerator) ? accelerator->feedback : NULL;
}

API_DECLARE_NONSTD(xint_t **) bli_accel_decode(netcode_t *codec, api_uint32_t seed, xint_t * encoded)
{
    bli_netcode_t *accelerator = (bli_netcode_t *)codec->third_param; 
    bli_base_decode(codec, seed, encoded);
    return BLI_IS_FAST(accelerator->coding_mode) ? decode_blocks(accelerator) : NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////

#if 1

/**
 * 初始化编码器组建
 * @param[in] codec 编码器对象
 * @param[in] param 参数指针
 * @remark 各编码器的初始化接口函数
 */
API_DECLARE(api_status_t) netcode_init(netcode_t *codec, void *param)
{
    bli_netcode_t *base = calloc(1, sizeof(bli_netcode_t));
    //------------------< 初始化参数 >------------------
    base->num = codec->row;
    base->size = codec->dimension;
    base->coding_mode = BLI_FAST;
    // Note the interpretation of density: 
	// - for a source node, density is the number of non-zero coefficients.
	// - for intermediate nodes, it is the maximum number of received coded
	// blocks to be linearly combined to build the new coded blocks. Actual
	// number of linear combination can be less than density if less number
	// of coded blocks are received so far.
	base->density = (int) (1 * base->num);
    
    codec->third_param = base;
    
    //------------------< 初始化回调函数 >------------------
    codec->alloc   = bli_alloc;
    codec->realloc = bli_realloc;
    codec->free    = bli_free;
    
    codec->destroy = bli_accel_destroy;
    codec->push    = bli_packet_add;
    codec->encode  = bli_accel_encode;
    codec->decode  = bli_accel_decode;
    bli_accel_init(codec);
    return API_SUCCESS;
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////
