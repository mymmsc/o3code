#include "bli_code.h"
//////////////////////////////////////////////////////////////////////////////////////////

#if NETCODE_USE_BLI

API_DECLARE_NONSTD(void *) bli_alloc(api_size_t s)
{
    return allocAlignedMem(s);
}

API_DECLARE_NONSTD(void *) bli_realloc(void *p, api_size_t s)
{
    return NULL;//_aligned_realloc(p, s, 16);
}

API_DECLARE_NONSTD(void) bli_free(void *p)
{
    freeAlignedMem(p);
}

static int g_nBase = 0;

API_DECLARE(api_status_t) bli_base_init(netcode_t *codec)
{
    bli_netcode_t *bcodec = codec->third_param;
    int   i;
    
    if (g_nBase == 0)
    {
		struct timeval t;
		gettimeofday(&t, NULL);
        srand((unsigned int)/*api_time_now()*/t.tv_sec);
		g_nBase = rand();
    }
    else
    {
        g_nBase = ((rand() % 1000 ) << 21) + ((rand() % 1000) << 13) + ((rand() % 1000) << 7) + rand();
    }
    bcodec->prEnc = calloc(1, sizeof(bli_random_t));
    bli_random_init(bcodec->prEnc);
    
    bcodec->prDec = calloc(1, sizeof(bli_random_t));
    bli_random_init(bcodec->prDec);
    
	// Initializing encode Random in case user wants to repeat an experiment.
	// Decoder's generator will receive its seed as part of data channel.
	if (g_nBase > 0 )
    {
        bli_random_init_ex(bcodec->prEnc, g_nBase);
    }
	// Keep track of the ownership of the invidual coefficients[i] memory area
	bcodec->memory_status = (api_status_t *)calloc(codec->row, sizeof(api_status_t));
    
    bcodec->coef = (xint_t **)calloc(codec->row, sizeof(xint_t *));
	// If non-recoding mode requires coefficients area unless override enabled. 
	for (i = 0; i < codec->row; i ++)
	{
		bcodec->coef[i] = (xint_t *) allocAlignedMem(codec->dimension * sizeof(xint_t));
		memset(bcodec->coef[i], 0x00, codec->dimension * sizeof(xint_t));
		// marked the memory as owned by bli
		bcodec->memory_status[i] = 1;
	}
    
    bcodec->feedback = (xint_t **)calloc(codec->row, sizeof(xint_t *));
    bcodec->colNonZero = (int *)calloc(codec->row, sizeof(int));
    bcodec->keep_coef = (xint_t *)calloc(codec->row, sizeof(xint_t));
    gf256_create(&(bcodec->gf));
    return API_SUCCESS;
}

static api_status_t bli_destroy(netcode_t *codec)
{
    int i;
    bli_netcode_t *bnc = codec->third_param;
    
    free(bnc->prEnc);
    bnc->prEnc = NULL;
    
    free(bnc->prDec);
    bnc->prDec = NULL;
    
	// if coefficients[i] owned by us, release the memory
	for (i = 0; i < bnc->num; i ++)
    {
		if (bnc->memory_status[i] == 1)
        {
			freeAlignedMem(bnc->coef[i]);
            bnc->coef[i] = NULL;
        }
    }
	free(bnc->coef);
    bnc->coef = NULL;
	free(bnc->memory_status);
    bnc->memory_status = NULL;
    free(bnc->keep_coef);
    bnc->keep_coef = NULL;
    free(bnc->feedback);
    bnc->feedback = NULL;
    free(bnc->colNonZero);
    bnc->colNonZero = NULL;
    gf256_destory(&(bnc->gf));
    
    return API_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * @return Whether the blocks are fully decoded.
 */
API_DECLARE(api_status_t) bli_isdecoded(bli_netcode_t *base)
{
	return (base->num == base->lastRow);
}

static void bli_reset(bli_netcode_t *base)
{
    int i;
    
	for (i = 0; i < base->num; i ++)
	{
		if ( base->memory_status[i] == 1 )
        {
			freeAlignedMem(base->coef[i]);
            base->coef[i] = NULL;
        }
	}
    
	memset(base->coef, 0x00, base->num * sizeof(api_byte_t *));
	// mark all as not owned by o3code
	memset(base->memory_status, 0x00, base->num * sizeof(api_byte_t));
	
	base->lastRow = 0;
}

API_DECLARE_NONSTD(api_status_t) bli_packet_add(netcode_t *codec, xint_t * packet)
{
    bli_netcode_t *base = codec->third_param;
	base->feedback[base->lastRow] = packet;
    base->lastRow++;
    return API_SUCCESS;
}

API_DECLARE(uint32_t) bli_coef_make(bli_netcode_t *base)
{
    // Preserve the seed of the random number generator.
    uint32_t s = base->prEnc->seed;
    int n_coef, i;
	int totalSelected = 0;
    
    // if this is a source node, encoding without having the fulll blocks doesn't 
	// make sense as we don't embed the number of current blocks in the encoded
	// block so receiver can't recover the correct original coefficients 
	
    {
		assert(base->lastRow == base->num);
		n_coef = base->num;
	}
	
    // Produce random coefficients for encoding.
    // density can be more than n_coef in recoding scenario
	if (base->density >= n_coef)
    {
        for (i = 0; i < n_coef; i++)
		{
            base->keep_coef[i] = (int) (bli_random_rand(base->prEnc) * 256);
		}
		
        return s;
    }
    
    // Compute random coefficients according to the density.
    if (base->density * 2 < n_coef)
    {
        memset(base->keep_coef, 0x00, n_coef);
        
        while (totalSelected < base->density)
        {
            int indexSelected = (int)(bli_random_rand(base->prEnc) * n_coef);
            if (base->keep_coef[indexSelected] == 0)
            {
                base->keep_coef[indexSelected] = (int) (bli_random_rand(base->prEnc) * 256);
                totalSelected ++;
            }
        }
    }
    else
    {
        for (i = 0; i < n_coef; i++)
        {
            base->keep_coef[i] = (int) (bli_random_rand(base->prEnc) * 256);
        }
        while (totalSelected < n_coef - base->density)
        {
            int indexSelected = (int)(bli_random_rand(base->prEnc) * n_coef);
            if (base->keep_coef[indexSelected] != 0)
            {
                base->keep_coef[indexSelected] = 0;
                totalSelected ++;
            }
        }
    }
    
    return s;
}

static void bli_coef_restore(bli_netcode_t *base, api_uint32_t seed)
{
    int totalSelected = 0;
    int i;
    
    // Recover random coefficients for decoding, using the
    // provided seed.
    bli_random_init_ex(base->prDec, seed);
    
    if (base->density == base->num)
    {
        for (i = 0; i < base->num; i++)
        {
            base->coef[base->lastRow][i] = (int) (bli_random_rand(base->prDec) * 256);
        }
        return;
    }
    
    // Compute random coefficients according to the density.
    if (base->density * 2 < base->num)
    {
		memset(base->coef[base->lastRow], 0x00, base->num);
        
        while (totalSelected < base->density)
        {
            int indexSelected = (int) (bli_random_rand(base->prDec) * base->num);
            if (base->coef[base->lastRow][indexSelected] == 0)
            {
                base->coef[base->lastRow][indexSelected] = (int) (bli_random_rand(base->prDec) * 256);
                totalSelected ++;
            }
        }
    }
    else
    {
        for (i = 0; i < base->num; i++)
        {
            base->coef[base->lastRow][i] = (int)(bli_random_rand(base->prDec) * 256);
        }
        while (totalSelected < base->num - base->density)
        {
            int indexSelected = (int)(bli_random_rand(base->prDec) * base->num);
            if (base->coef[base->lastRow][indexSelected] != 0)
            {
                base->coef[base->lastRow][indexSelected] = 0;
                totalSelected ++;
            }
        }
    }
}

static api_uint32_t bli_encode(netcode_t *codec, api_byte_t * outcome)
{
    api_uint32_t s;
    int z, i;
    bli_netcode_t *base = (bli_netcode_t *)codec->third_param;
    s = bli_coef_make(base);
    
    for (z = 0; z < base->size; z ++)
    {
        outcome[z] = 0;
        for (i = 0; i < base->lastRow; i ++)
        {
            outcome[z] ^= gf256_base_multiply(base->gf, base->keep_coef[i], base->feedback[i][z]);
        }
    }
    
    return s;
}

/**
 * Decode blocks in decodedSoFar.
 * @return decoded the decoded data blocks if decoding is successful. 
 */
static xint_t ** decode_blocks(bli_netcode_t *base)
{
    int i, j, currentRow;
    xint_t factor;
    xint_t * t;
    
    // A. Reduce leading (and possibly trailing) coefficients in the
    // last row to zero.
    for (i = 0; i < base->lastRow; i ++)
    {
        factor = base->coef[base->lastRow][base->colNonZero[i]];
        for (j = 0; j < base->num; j ++)
        {
            base->coef[base->lastRow][j] ^= gf256_base_multiply(base->gf, base->coef[i][j],
                                        factor);
        }
        for (j = 0; j < base->size; j ++)
        {
            base->feedback[base->lastRow][j] ^= gf256_base_multiply(base->gf, base->feedback[i][j],
                                        factor);
        }
    }
    
    // B. Find the leading non-zero coefficient in the current row.
    factor = 0;
    
    for (i = 0; i < base->num; i ++)
    {
        if (base->coef[base->lastRow][i] != 0)
        {
            factor = base->coef[base->lastRow][i];
            base->colNonZero[base->lastRow] = i;
            break;
        }
    }
    
    // C. Check for linear independence with rows above this one.
    if (factor == 0)
    {
        // a highly unlikely event if coefficients are randomly generated.
        return BLI_LINEAR_DEPENDENT;
    }
    
    // D. Reduce the leading non-zero entry to 1, such that the result
    // conforms to the row echelon form (REF).
    for (i = base->colNonZero[base->lastRow]; i < base->num; i ++)
    {
        base->coef[base->lastRow][i] = gf256_base_divide(base->gf, base->coef[base->lastRow][i],
                                   factor);
    }
    for (i = 0; i < base->size; i ++)
    {
        base->feedback[base->lastRow][i] = gf256_base_divide(base->gf, base->feedback[base->lastRow][i],
                                   factor);
    }
    
    // E. Reduce the coefficient matrix to the reduced row-echelon form (RREF).
    
    // E.1. Reduce to zeros in entries above columnNonZero[lastRow].
    for (i = 0; i < base->lastRow; i ++)
    {
        factor = base->coef[i][base->colNonZero[base->lastRow]];
        
        for (j = base->colNonZero[i]; j < base->num; j ++)
        {
            base->coef[i][j] ^= gf256_base_multiply(base->gf, base->coef[base->lastRow][j],
                                  factor);
        }
        for (j = 0; j < base->size; j ++)
        {
            base->feedback[i][j] ^= gf256_base_multiply(base->gf, base->feedback[base->lastRow][j],
                                  factor);
        }
    }
    
    // E.2. Reorder rows based on columnNonZero[].
    currentRow = base->lastRow;
    while (currentRow > 0 && base->colNonZero[currentRow] < base->colNonZero[currentRow - 1])
    {
        api_uint32_t c;
        // Exchange two rows.
        t = base->coef[currentRow - 1];
        base->coef[currentRow - 1] = base->coef[currentRow];
        base->coef[currentRow] = t;
        
        t = base->feedback[currentRow - 1];
        base->feedback[currentRow - 1] = base->feedback[currentRow];
        base->feedback[currentRow] = t;
        
        c = base->colNonZero[currentRow - 1];
        base->colNonZero[currentRow - 1] = base->colNonZero[currentRow];
        base->colNonZero[currentRow] = c;
        
        currentRow --;
    }
    
    base->lastRow ++;
    
    return bli_isdecoded(base) ? base->feedback : NULL;
}

API_DECLARE_NONSTD(xint_t **) bli_base_decode(netcode_t *codec, uint32_t seed, xint_t * encoded)
{
    bli_netcode_t *base = codec->third_param;
    assert (base->lastRow < base->num);
    if (seed >= 0)
	{
		// if in embedded mode and receiving a seed-based coded block, 
		// allocate memory for the coefficient area if not already there... 
		if ( base->memory_status[base->lastRow] == 0 )
		{
			assert (base->coef[base->lastRow] == NULL);
			base->coef[base->lastRow] = (api_byte_t *) allocAlignedMem(base->num);
			base->memory_status[base->lastRow] = 1;
		}
		
		bli_coef_restore(base, seed);
		base->feedback[base->lastRow] = encoded;
    }
	else
    {
		assert (base->coef[base->lastRow] == NULL);
        // Set the coefficients of the given encoded block
		base->coef[base->lastRow] = encoded;
		// Data area follows the coefficients 
	    base->feedback[base->lastRow] = encoded + base->num;
	}
    
    return BLI_IS_BASE(base->coding_mode) ? decode_blocks(base) : NULL;
}

#if 0

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
    base->coding_mode = BLI_BASE;
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
    
    codec->destroy = bli_destroy;
    codec->push    = bli_packet_add;
    codec->encode  = bli_encode;
    codec->decode  = bli_base_decode;
    bli_base_init(codec);
    return API_SUCCESS;
}

#endif

#endif /* ! NETCODE_USE_BLI */
