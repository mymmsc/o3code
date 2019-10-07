#include "netcode.h"

/**
 * 创建一个编码器
 */
API_DECLARE(api_status_t) netcode_create(netcode_t *codec, api_uint32_t n, api_uint32_t row)
{
    api_status_t rv = API_SUCCESS;
    
    memset(codec, 0x00, sizeof(*codec));
    codec->dimension = n;
    codec->row = row;
    
    netcode_init(codec, NULL);
    
    if (codec->radix == NULL)
    {
        codec->radix = (xint_t **)calloc(codec->row, sizeof(xint_t *));
    }
    
    if (codec->radix == NULL)
    {
        rv = API_ENOMEM;
    }
    else
    {
        int i;
        for (i = 0; i < codec->row; i++)
        {
            codec->radix[i] = codec->alloc(codec->dimension * sizeof(xint_t));
        }
    }
    return rv;
}

/**
 * 注销一个编码器
 */
API_DECLARE(api_status_t) netcode_close(netcode_t *codec)
{
    int i = 0;
    if (codec->radix != NULL)
    {
        for (i = 0; i < codec->row; i++)
        {
            if (codec->radix[i] != NULL)
            {
                codec->free(codec->radix[i]);
                codec->radix[i] = NULL;
            }
        }
        free(codec->radix);
        codec->radix = NULL;
    }
    
    return API_SUCCESS;
}

/**
 * 编码器复位
 * @param[in] codec 编码器
 */
API_DECLARE(void) netcode_reset(netcode_t *codec)
{
    codec->row_pos = 0;
}

/**
 * 添加一个基
 * @param[in] codec 编码器
 * @param[in] in 一个基的数组
 */
API_DECLARE(api_status_t) netcode_push(netcode_t *codec, xint_t *in)
{
#if 0
    api_status_t rv = API_SUCCESS;
    assert(codec != NULL && in != NULL);
    if (codec->row_pos < codec->row)
    {
        memcpy(codec->radix[codec->row_pos++], in, codec->dimension);
    }
    
    return rv;
#endif
    return codec->push(codec, in);
}

/**
 * 编码输出一个基
 * @param[in] codec 编码器
 * @param[out] seed 基的种子
 * @param[out] out 一个基数组
 */
API_DECLARE(api_status_t) netcode_encode(netcode_t *codec, api_uint32_t *seed, xint_t *out)
{
    *seed = codec->encode(codec, out);
    return API_SUCCESS;
}

/**
 * 解码码一个基
 * @param[in] codec 编码器
 * @param[in] seed 基的种子
 * @param[in] in 一个基
 * @param[out] out 二维数组, 如果解码失败输出NULL, 否则解码成功
 * @return 返回API_EAGAIN表示线性相关
 */
API_DECLARE(api_status_t) netcode_decode(netcode_t *codec, api_uint32_t seed, xint_t *in, xint_t ***out)
{
    *out = codec->decode(codec, seed, in);
    return *out == (xint_t **)(-1) ? API_EAGAIN : API_SUCCESS;
}
