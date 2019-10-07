// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/**
 * @file   netcode.h
 * @brief  AXIS Linear Network Coding Operations
 * @author Copyright (C) 2000-2007, mymmsc.com  All rights reserved.
 * @author Feng Wang wangfeng@yeah.net
 * @date   Nov 29, 2007
 * @since  V1.0.1-DEV ��һ�������汾
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

/** ʹ�����ᴺV13�汾��NetLib, Baochun Li, bli@eecg.toronto.edu */
#define NETCODE_USE_BLI 1
/** ʹ��wei dai�汾��shark, ���FreeBSD /usr/ports/net-p2p/xmule */
#define NETCODE_USE_WD  0
/** ʹ������V1.0.1-DEV�汾��O3Code, Feng Wang, wangfeng@yeah.net */
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
    xint_t      **radix;       /**< row�� */
    int           dimension;   /**< ά�� */
    int           row;         /**< ���� */
    int           row_pos;     /**< ����ָ�� */
    void         *third_param; /**< ���������� */
    
    /**
     * ��ʼ��һ��������
     * @param[in] codec ������
     * @param[in] param ����ָ��
     */
    api_status_t (*init)   (netcode_t *codec, void *param);
    
    /**
     * ����һ��������
     * @param[in] codec ������
     */
    api_status_t (*destroy)(netcode_t *codec);
    
    /**
     * ���һ����
     * @param[in] codec ������
     * @param[in] in һ����������
     */
    api_status_t (*push)   (netcode_t *codec, xint_t *in);
    
    /**
     * �������һ����
     * @param[in] codec ������
     * @param[out] out һ��������
     * @return seed, ��������
     */
    api_uint32_t (*encode) (netcode_t *codec, xint_t *out);
    
    /**
     * ������һ����
     * @param[in] codec ������
     * @param[in] seed ��������
     * @param[in] in һ����
     * @return ��ά����, �������ʧ�����NULL, �������ɹ�
     */
    xint_t ** (*decode) (netcode_t *codec, uint32_t seed, xint_t *in);
    
    void * (*alloc)   (api_size_t s);
    void * (*realloc) (void *p, api_size_t s);
    void   (*free)    (void *p);
};

/**
 * ��ʼ���������齨
 * @param[in] codec ����������
 * @param[in] param ����ָ��
 * @remark ���������ĳ�ʼ���ӿں���
 */
API_DECLARE(api_status_t) netcode_init(netcode_t *codec, void *param);

/**
 * ����һ��������
 * @param[in] codec ������
 * @param[in] n ά��
 * @param[in] row ����
 */
API_DECLARE(api_status_t) netcode_create(netcode_t *codec, uint32_t n, uint32_t row);

/**
 * ע��һ��������
 * @param[in] codec ������
 */
API_DECLARE(api_status_t) netcode_close(netcode_t *codec);

/**
 * ��������λ
 * @param[in] codec ������
 */
API_DECLARE(void) netcode_reset(netcode_t *codec);

/**
 * �����Ƿ�ɹ�
 * @param[in] codec ������
 */
#define netcode_isdecoded(codec) ((codec)->row == (codec)->row_pos)

/**
 * ���һ����
 * @param[in] codec ������
 * @param[in] in һ����������
 */
API_DECLARE(api_status_t) netcode_push(netcode_t *codec, xint_t *in);

/**
 * �������һ����
 * @param[in] codec ������
 * @param[out] seed ��������
 * @param[out] out һ��������
 * @return ����API_EAGAIN��ʾ�������
 */
API_DECLARE(api_status_t) netcode_encode(netcode_t *codec, uint32_t *seed, xint_t *out);

/**
 * ������һ����
 * @param[in] codec ������
 * @param[in] seed ��������
 * @param[in] in һ����
 * @param[out] out ��ά����, �������ʧ�����NULL, �������ɹ�
 * @return ����API_EAGAIN��ʾ�������
 */
API_DECLARE(api_status_t) netcode_decode(netcode_t *codec, uint32_t seed, xint_t *in, xint_t ***out);

/** @} */

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* ! O3CODE_NETCODE_H */
