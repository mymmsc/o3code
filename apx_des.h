/* d3des.h -
 *
 *	Headers and defines for d3des.c
 *	Graven Imagery, 1992.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge
 *	(GEnie : OUTER; CIS : [71755,204])
 */
#ifndef APX_CRYPTO_DES_H
#define APX_CRYPTO_DES_H
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

#define D2_DES	    /* include double-length support */
#define D3_DES		/* include triple-length support */

#ifdef D3_DES
#  ifndef D2_DES
#    define D2_DES		/* D2_DES is needed for D3_DES */
#  endif
#endif

#define DES_ENCRYPT	0	/* MODE == encrypt */
#define DES_DECRYPT	1	/* MODE == decrypt */

typedef struct  
{
    unsigned long KnL[32];
    unsigned long KnR[32];
    unsigned long Kn3[32];
}apx_des_ctx_t;

extern void apx_des_init(apx_des_ctx_t *dc, short mode, unsigned char *key, int keylen);
extern void apx_des(apx_des_ctx_t *dc, unsigned char *to, unsigned char *from, int len);

/* A useful alias on 68000-ish machines, but NOT USED HERE. */

typedef union {
    unsigned long blok[2];
    unsigned short word[4];
    unsigned char byte[8];
} M68K;

/*	          hexkey[8]     MODE
 * Sets the internal key register according to the hexadecimal
 * key contained in the 8 bytes of hexkey, according to the DES,
 * for encryption or decryption according to MODE.
 */
extern void deskey(apx_des_ctx_t *dc, unsigned char *, short);

/*		    cookedkey[32]
 * Loads the internal key register with the data in cookedkey.
 */
extern void usekey(apx_des_ctx_t *dc, unsigned long *);

/*		   cookedkey[32]
 * Copies the contents of the internal key register into the storage
 * located at &cookedkey[0].
 */
extern void cpkey(apx_des_ctx_t *dc, unsigned long *);

/*		    from[8]	      to[8]
 * Encrypts/Decrypts (according to the key currently loaded in the
 * internal key register) one block of eight bytes at address 'from'
 * into the block at address 'to'.  They can be the same.
 */
extern void des(apx_des_ctx_t *dc, unsigned char *, unsigned char *);

#ifdef D2_DES

#define desDkey(a,b)	des2key((a),(b))
extern void des2key(apx_des_ctx_t *dc, unsigned char *, short);
/*		      hexkey[16]     MODE
 * Sets the internal key registerS according to the hexadecimal
 * keyS contained in the 16 bytes of hexkey, according to the DES,
 * for DOUBLE encryption or decryption according to MODE.
 * NOTE: this clobbers all three key registers!
 */

extern void Ddes(apx_des_ctx_t *dc, unsigned char *, unsigned char *);
/*		    from[8]	      to[8]
 * Encrypts/Decrypts (according to the keyS currently loaded in the
 * internal key registerS) one block of eight bytes at address 'from'
 * into the block at address 'to'.  They can be the same.
 */

extern void D2des(apx_des_ctx_t *dc, unsigned char *, unsigned char *);
/*		    from[16]	      to[16]
 * Encrypts/Decrypts (according to the keyS currently loaded in the
 * internal key registerS) one block of SIXTEEN bytes at address 'from'
 * into the block at address 'to'.  They can be the same.
 */

extern void makekey(apx_des_ctx_t *dc, char *, unsigned char *);
/*		*password,	single-length key[8]
 * With a double-length default key, this routine hashes a NULL-terminated
 * string into an eight-byte random-looking key, suitable for use with the
 * deskey() routine.
 */

#define makeDkey(d, a, b)	make2key((d), (a),(b))
extern void make2key(apx_des_ctx_t *dc, char *, unsigned char *);
/*		*password,	double-length key[16]
 * With a double-length default key, this routine hashes a NULL-terminated
 * string into a sixteen-byte random-looking key, suitable for use with the
 * des2key() routine.
 */

#ifndef D3_DES	/* D2_DES only */

#define useDkey(a)	use2key((a))
#define cpDkey(a)	cp2key((a))

/*		    cookedkey[64]
 * Loads the internal key registerS with the data in cookedkey.
 * NOTE: this clobbers all three key registers!
 */
extern void use2key(unsigned long *);

/*		   cookedkey[64]
 * Copies the contents of the internal key registerS into the storage
 * located at &cookedkey[0].
 */
extern void cp2key(unsigned long *);

#else	/* D3_DES too */

#define useDkey(d, a)	use3key((d), (a))
#define cpDkey(d, a)	cp3key((d), (a))

extern void des3key(apx_des_ctx_t *dc, unsigned char *, short);
/*		      hexkey[24]     MODE
 * Sets the internal key registerS according to the hexadecimal
 * keyS contained in the 24 bytes of hexkey, according to the DES,
 * for DOUBLE encryption or decryption according to MODE.
 */

extern void use3key(apx_des_ctx_t *dc, unsigned long *);
/*		    cookedkey[96]
 * Loads the 3 internal key registerS with the data in cookedkey.
 */

extern void cp3key(apx_des_ctx_t *dc, unsigned long *);
/*		   cookedkey[96]
 * Copies the contents of the 3 internal key registerS into the storage
 * located at &cookedkey[0].
 */

extern void make3key(apx_des_ctx_t *dc, char *, unsigned char *);
/*		*password,	triple-length key[24]
 * With a triple-length default key, this routine hashes a NULL-terminated
 * string into a twenty-four-byte random-looking key, suitable for use with
 * the des3key() routine.
 */

#endif	/* D3_DES */
#endif	/* D2_DES */

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* APX_CRYPTO_DES_H */
