// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/**
 * @file   netcode.h
 * @brief  AXIS Linear Network Coding Operations
 * @author Copyright (C) 2000-2007, mymmsc.com  All rights reserved.
 * @author Feng Wang wangfeng@yeah.net
 * @date   Nov 29, 2007
 * @since  V1.0.1-DEV 第一个开发版本
 */

#ifndef O3CODE_NETCODE_H
#define O3CODE_NETCODE_H
//////////////////////////////////////////////////////////////////////////////////////////

#include <api/errno.h>
//#include <api_time.h>
#include <memalign.h>
#include <assert.h>
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup NetCode
 * @ingroup NetCode 
 * @{
 */

//////////////////////////////////////////////////////////////////////////////////////////

/** 使用李葆春V13版本的NetLib, Baochun Li, bli@eecg.toronto.edu */
#define NETCODE_USE_BLI 1
/** 使用wei dai版本的shark, 详见FreeBSD /usr/ports/net-p2p/xmule */
#define NETCODE_USE_WD  0
/** 使用王锋V1.0.1-DEV版本的O3Code, Feng Wang, wangfeng@yeah.net */
#define NETCODE_USE_WF  0
//////////////////////////////////////////////////////////////////////////////////////////

#define NETCODE_INTEGER_USE_BITS 8
#define NETCODE_INTEGER_TYPE (NETCODE_INTEGER_USE_BITS / 8)

#if NETCODE_INTEGER_TYPE == 8
typedef uint64_t xint_t;
#elif NETCODE_INTEGER_TYPE == 4
typedef uint32_t xint_t;
#elif NETCODE_INTEGER_TYPE == 2
typedef uint16_t xint_t;
#else //NETCODE_INTEGER_TYPE == 1
typedef byte_t xint_t;
#endif

//////////////////////////////////////////////////////////////////////////////////////////

typedef struct netcode_t netcode_t;
struct netcode_t 
{
    xint_t      **radix;       /**< row基 */
    int           dimension;   /**< 维数 */
    int           row;         /**< 行数 */
    int           row_pos;     /**< 行数指针 */
    void         *third_param; /**< 编码器参数 */
    
    /**
     * 初始化一个编码器
     * @param[in] codec 编码器
     * @param[in] param 参数指针
     */
    api_status_t (*init)   (netcode_t *codec, void *param);
    
    /**
     * 销毁一个编码器
     * @param[in] codec 编码器
     */
    api_status_t (*destroy)(netcode_t *codec);
    
    /**
     * 添加一个基
     * @param[in] codec 编码器
     * @param[in] in 一个基的数组
     */
    api_status_t (*push)   (netcode_t *codec, xint_t *in);
    
    /**
     * 编码输出一个基
     * @param[in] codec 编码器
     * @param[out] out 一个基数组
     * @return seed, 基的种子
     */
    api_uint32_t (*encode) (netcode_t *codec, xint_t *out);
    
    /**
     * 解码码一个基
     * @param[in] codec 编码器
     * @param[in] seed 基的种子
     * @param[in] in 一个基
     * @return 二维数组, 如果解码失败输出NULL, 否则解码成功
     */
    xint_t ** (*decode) (netcode_t *codec, uint32_t seed, xint_t *in);
    
    void * (*alloc)   (api_size_t s);
    void * (*realloc) (void *p, api_size_t s);
    void   (*free)    (void *p);
};

/**
 * 初始化编码器组建
 * @param[in] codec 编码器对象
 * @param[in] param 参数指针
 * @remark 各编码器的初始化接口函数
 */
API_DECLARE(api_status_t) netcode_init(netcode_t *codec, void *param);

/**
 * 创建一个编码器
 * @param[in] codec 编码器
 * @param[in] n 维数
 * @param[in] row 行数
 */
API_DECLARE(api_status_t) netcode_create(netcode_t *codec, uint32_t n, uint32_t row);

/**
 * 注销一个编码器
 * @param[in] codec 编码器
 */
API_DECLARE(api_status_t) netcode_close(netcode_t *codec);

/**
 * 编码器复位
 * @param[in] codec 编码器
 */
API_DECLARE(void) netcode_reset(netcode_t *codec);

/**
 * 解码是否成功
 * @param[in] codec 编码器
 */
#define netcode_isdecoded(codec) ((codec)->row == (codec)->row_pos)

/**
 * 添加一个基
 * @param[in] codec 编码器
 * @param[in] in 一个基的数组
 */
API_DECLARE(api_status_t) netcode_push(netcode_t *codec, xint_t *in);

/**
 * 编码输出一个基
 * @param[in] codec 编码器
 * @param[out] seed 基的种子
 * @param[out] out 一个基数组
 * @return 返回API_EAGAIN表示线性相关
 */
API_DECLARE(api_status_t) netcode_encode(netcode_t *codec, uint32_t *seed, xint_t *out);

/**
 * 解码码一个基
 * @param[in] codec 编码器
 * @param[in] seed 基的种子
 * @param[in] in 一个基
 * @param[out] out 二维数组, 如果解码失败输出NULL, 否则解码成功
 * @return 返回API_EAGAIN表示线性相关
 */
API_DECLARE(api_status_t) netcode_decode(netcode_t *codec, uint32_t seed, xint_t *in, xint_t ***out);

/** @} */

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* ! O3CODE_NETCODE_H */
