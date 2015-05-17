/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2015 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: François Poirotte <clicky@erebot.net>                        |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if HAVE_LIBTOMCRYPT

#include <tomcrypt.h>
#include "php_tomcrypt.h"
#include "ext/standard/file.h"

typedef struct _php_tomcrypt_rng {
	char        *name;
	prng_state   state;
} php_tomcrypt_rng;

static php_tomcrypt_rng php_tomcrypt_rngs[] = {
#ifdef LTC_YARROW
	{.name = PHP_TOMCRYPT_RNG_YARROW},
#endif
#ifdef LTC_RC4
	{ .name =  PHP_TOMCRYPT_RNG_RC4 },
#endif
#ifdef LTC_FORTUNA
	{ .name = PHP_TOMCRYPT_RNG_FORTUNA },
#endif
#ifdef LTC_SOBER128
	{ .name = PHP_TOMCRYPT_RNG_SOBER128 },
#endif
#ifdef LTC_SPRNG
	{ .name = PHP_TOMCRYPT_RNG_SECURE },
#endif
	{NULL}
};

#if (PHP_MAJOR_VERSION >= 5)
# define DEFAULT_CONTEXT FG(default_context)
#else
# define DEFAULT_CONTEXT NULL
#endif


/* {{{ arginfo */

/* Misc. */
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_strerror, 0, 0, 1)
	ZEND_ARG_INFO(0, errno)
ZEND_END_ARG_INFO()

#ifdef LTC_BASE64
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_base64_encode, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_base64_decode, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()
#endif


/* Lists */
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_list_modes, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_list_ciphers, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_list_hashes, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_list_macs, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_list_rngs, 0, 0, 0)
ZEND_END_ARG_INFO()


/* Ciphers */
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_name, 0, 0, 1)
	ZEND_ARG_INFO(0, cipher)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_block_size, 0, 0, 1)
	ZEND_ARG_INFO(0, cipher)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_adapt_key_size, 0, 0, 2)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, keysize)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_min_key_size, 0, 0, 1)
	ZEND_ARG_INFO(0, cipher)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_max_key_size, 0, 0, 1)
	ZEND_ARG_INFO(0, cipher)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_default_rounds, 0, 0, 1)
	ZEND_ARG_INFO(0, cipher)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_encrypt, 0, 0, 4)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, plaintext)
	ZEND_ARG_INFO(0, mode)
	ZEND_ARG_INFO(0, options) /* array */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cipher_decrypt, 0, 0, 4)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, ciphertext)
	ZEND_ARG_INFO(0, mode)
	ZEND_ARG_INFO(0, options) /* array */
ZEND_END_ARG_INFO()


/* Hashes */
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hash_name, 0, 0, 1)
	ZEND_ARG_INFO(0, hash)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hash_block_size, 0, 0, 1)
	ZEND_ARG_INFO(0, hash)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hash_digest_size, 0, 0, 1)
	ZEND_ARG_INFO(0, hash)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hash_string, 0, 0, 2)
	ZEND_ARG_INFO(0, algo)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hash_file, 0, 0, 2)
	ZEND_ARG_INFO(0, algo)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()


/* xMAC */
#ifdef LTC_HMAC
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hmac_string, 0, 0, 3)
	ZEND_ARG_INFO(0, hash)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_hmac_file, 0, 0, 3)
	ZEND_ARG_INFO(0, hash)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()
#endif

#ifdef LTC_OMAC
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cmac_string, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_cmac_file, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()
#endif

#ifdef LTC_PMAC
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_pmac_string, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_pmac_file, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()
#endif

#ifdef LTC_PELICAN
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_pelican_string, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_pelican_file, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()
#endif

#ifdef LTC_XCBC
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_xcbc_string, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_xcbc_file, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()
#endif

#ifdef LTC_F9_MODE
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_f9_string, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_f9_file, 0, 0, 3)
	ZEND_ARG_INFO(0, cipher)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, raw_output)
ZEND_END_ARG_INFO()
#endif

/* PRNGs */
ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_rng_name, 0, 0, 1)
	ZEND_ARG_INFO(0, prng)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tomcrypt_rng_get_bytes, 0, 0, 1)
	ZEND_ARG_INFO(0, size)
	ZEND_ARG_INFO(0, prng)
ZEND_END_ARG_INFO()

/* }}} */

const zend_function_entry tomcrypt_functions[] = { /* {{{ */
	/* Misc. */
	PHP_FE(tomcrypt_strerror,				arginfo_tomcrypt_strerror)
#ifdef LTC_BASE64
	PHP_FE(tomcrypt_base64_encode,			arginfo_tomcrypt_base64_encode)
	PHP_FE(tomcrypt_base64_decode,			arginfo_tomcrypt_base64_decode)
#endif

	/* Lists */
	PHP_FE(tomcrypt_list_modes,				arginfo_tomcrypt_list_modes)
	PHP_FE(tomcrypt_list_ciphers,			arginfo_tomcrypt_list_ciphers)
	PHP_FE(tomcrypt_list_hashes,			arginfo_tomcrypt_list_hashes)
	PHP_FE(tomcrypt_list_macs,				arginfo_tomcrypt_list_macs)
	PHP_FE(tomcrypt_list_rngs,				arginfo_tomcrypt_list_rngs)

	/* Ciphers */
	PHP_FE(tomcrypt_cipher_name,			arginfo_tomcrypt_cipher_name)
	PHP_FE(tomcrypt_cipher_block_size,		arginfo_tomcrypt_cipher_block_size)
	PHP_FE(tomcrypt_cipher_adapt_key_size,	arginfo_tomcrypt_cipher_adapt_key_size)
	PHP_FE(tomcrypt_cipher_min_key_size,	arginfo_tomcrypt_cipher_min_key_size)
	PHP_FE(tomcrypt_cipher_max_key_size,	arginfo_tomcrypt_cipher_max_key_size)
	PHP_FE(tomcrypt_cipher_default_rounds,	arginfo_tomcrypt_cipher_default_rounds)
	PHP_FE(tomcrypt_cipher_encrypt,			arginfo_tomcrypt_cipher_encrypt)
	PHP_FE(tomcrypt_cipher_decrypt,			arginfo_tomcrypt_cipher_decrypt)

	/* Hashes */
	PHP_FE(tomcrypt_hash_name,				arginfo_tomcrypt_hash_name)
	PHP_FE(tomcrypt_hash_block_size,		arginfo_tomcrypt_hash_block_size)
	PHP_FE(tomcrypt_hash_digest_size,		arginfo_tomcrypt_hash_digest_size)
	PHP_FE(tomcrypt_hash_string,			arginfo_tomcrypt_hash_string)
	PHP_FE(tomcrypt_hash_file,				arginfo_tomcrypt_hash_file)

	/* xMAC */
#ifdef LTC_HMAC
	PHP_FE(tomcrypt_hmac_string,			arginfo_tomcrypt_hmac_string)
	PHP_FE(tomcrypt_hmac_file,				arginfo_tomcrypt_hmac_file)
#endif
#ifdef LTC_OMAC
	PHP_FE(tomcrypt_cmac_string,			arginfo_tomcrypt_cmac_string)
	PHP_FE(tomcrypt_cmac_file,				arginfo_tomcrypt_cmac_file)
#endif
#ifdef LTC_PMAC
	PHP_FE(tomcrypt_pmac_string,			arginfo_tomcrypt_pmac_string)
	PHP_FE(tomcrypt_pmac_file,				arginfo_tomcrypt_pmac_file)
#endif
#ifdef LTC_PELICAN
	PHP_FE(tomcrypt_pelican_string,			arginfo_tomcrypt_pelican_string)
	PHP_FE(tomcrypt_pelican_file,			arginfo_tomcrypt_pelican_file)
#endif
#ifdef LTC_XCBC
	PHP_FE(tomcrypt_xcbc_string,			arginfo_tomcrypt_xcbc_string)
	PHP_FE(tomcrypt_xcbc_file,				arginfo_tomcrypt_xcbc_file)
#endif
#ifdef LTC_F9_MODE
	PHP_FE(tomcrypt_f9_string,				arginfo_tomcrypt_f9_string)
	PHP_FE(tomcrypt_f9_file,				arginfo_tomcrypt_f9_file)
#endif

	/* PRNGs */
	PHP_FE(tomcrypt_rng_name, 				arginfo_tomcrypt_rng_name)
	PHP_FE(tomcrypt_rng_get_bytes, 			arginfo_tomcrypt_rng_get_bytes)

	PHP_FE_END
};
/* }}} */

static PHP_MINFO_FUNCTION(tomcrypt);
static PHP_MINIT_FUNCTION(tomcrypt);
static PHP_MSHUTDOWN_FUNCTION(tomcrypt);

zend_module_entry tomcrypt_module_entry = {
	STANDARD_MODULE_HEADER,
	"tomcrypt",
	tomcrypt_functions,
	PHP_MINIT(tomcrypt),
	PHP_MSHUTDOWN(tomcrypt),
	NULL, /* RINIT */
	NULL, /* RSHUTDOWN */
	PHP_MINFO(tomcrypt),
	PHP_TOMCRYPT_VERSION,
	STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_TOMCRYPT
ZEND_GET_MODULE(tomcrypt)
#endif

#define TOMCRYPT_ADD_CIPHER(cname, desc) \
	if (register_cipher(&desc) == -1) { \
		return FAILURE; \
	} \
	REGISTER_STRING_CONSTANT("TOMCRYPT_CIPHER_" #cname, desc.name, CONST_PERSISTENT)

#define TOMCRYPT_ADD_HASH(cname, desc) \
	if (register_hash(&desc) == -1) { \
		return FAILURE; \
	} \
	REGISTER_STRING_CONSTANT("TOMCRYPT_HASH_" #cname, desc.name, CONST_PERSISTENT)

#define TOMCRYPT_ADD_MODE(mode) \
	REGISTER_STRING_CONSTANT("TOMCRYPT_MODE_" #mode, PHP_TOMCRYPT_MODE_ ##mode , CONST_PERSISTENT)

#define TOMCRYPT_ADD_MAC(mac) \
	REGISTER_STRING_CONSTANT("TOMCRYPT_MAC_" #mac, PHP_TOMCRYPT_MAC_ ##mac , CONST_PERSISTENT)

#define TOMCRYPT_ADD_RNG(rng, desc) \
	if (register_prng(&desc) == -1) { \
		return FAILURE; \
	} \
	REGISTER_STRING_CONSTANT("TOMCRYPT_RNG_" #rng, PHP_TOMCRYPT_RNG_ ##rng , CONST_PERSISTENT)

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(tomcrypt)
{
	int i;

/*	TOMCRYPT_CIPHER(ARCFOUR_IV, "arcfour-iv");*/
/*	TOMCRYPT_CIPHER(ARCFOUR, "arcfour");*/
/*	TOMCRYPT_CIPHER(CAST_128, "cast-128");*/
/*	TOMCRYPT_CIPHER(CAST_256, "cast-256");*/
/*	TOMCRYPT_CIPHER(CRYPT, "crypt");*/
/*	TOMCRYPT_CIPHER(ENIGNA, "crypt");*/
/*	TOMCRYPT_CIPHER(GOST, "gost");*/
/*	TOMCRYPT_CIPHER(LOKI97, "loki97");*/
/*	TOMCRYPT_CIPHER(PANAMA, "panama");*/
/*	TOMCRYPT_CIPHER(SERPENT, "serpent");*/
/*	TOMCRYPT_CIPHER(THREEWAY, "threeway");*/
/*	TOMCRYPT_CIPHER(WAKE, "wake");*/

/*	TOMCRYPT_CIPHER(IDEA, "idea");*/
/*	TOMCRYPT_CIPHER(MARS, "mars");*/


	/* Cipher modes */
#ifdef LTC_ECB_MODE
	TOMCRYPT_ADD_MODE(ECB);
#endif
#ifdef LTC_CFB_MODE
	TOMCRYPT_ADD_MODE(CFB);
#endif
#ifdef LTC_OFB_MODE
	TOMCRYPT_ADD_MODE(OFB);
#endif
#ifdef LTC_CBC_MODE
	TOMCRYPT_ADD_MODE(CBC);
#endif
#ifdef LTC_CTR_MODE
	TOMCRYPT_ADD_MODE(CTR);
	REGISTER_LONG_CONSTANT("TOMCRYPT_CTR_LITTLE_ENDIAN", CTR_COUNTER_LITTLE_ENDIAN, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("TOMCRYPT_CTR_BIG_ENDIAN", CTR_COUNTER_BIG_ENDIAN, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("TOMCRYPT_CTR_RFC3686", LTC_CTR_RFC3686, CONST_PERSISTENT);
#endif
#ifdef LTC_LRW_MODE
	TOMCRYPT_ADD_MODE(LRW);
#endif
#ifdef LTC_F8_MODE
	TOMCRYPT_ADD_MODE(F8);
#endif
#ifdef LTC_XTS_MODE
	TOMCRYPT_ADD_MODE(XTS);
#endif
#ifdef LTC_CCM_MODE
	TOMCRYPT_ADD_MODE(CCM);
#endif
#ifdef LTC_GCM_MODE
	TOMCRYPT_ADD_MODE(GCM);
#endif
#ifdef LTC_EAX_MODE
	TOMCRYPT_ADD_MODE(EAX);
#endif
#ifdef LTC_OCB_MODE
	TOMCRYPT_ADD_MODE(OCB);
#endif


	/* Ciphers */
#ifdef LTC_BLOWFISH
	TOMCRYPT_ADD_CIPHER(BLOWFISH, blowfish_desc);
#endif
#ifdef LTC_RC5
	TOMCRYPT_ADD_CIPHER(RC5, rc5_desc);
#endif
#ifdef LTC_RC6
	TOMCRYPT_ADD_CIPHER(RC6, rc6_desc);
#endif
#ifdef LTC_RC2
	TOMCRYPT_ADD_CIPHER(RC2, rc2_desc);
#endif
#ifdef LTC_SAFERP
	TOMCRYPT_ADD_CIPHER(SAFERPLUS, saferp_desc);
#endif
#ifdef LTC_SAFER
	TOMCRYPT_ADD_CIPHER(SAFERK64, safer_k64_desc);
	TOMCRYPT_ADD_CIPHER(SAFERK128, safer_k128_desc);
	TOMCRYPT_ADD_CIPHER(SAFERSK64, safer_sk64_desc);
	TOMCRYPT_ADD_CIPHER(SAFERSK128, safer_sk128_desc);
	TOMCRYPT_ADD_CIPHER(SAFER64, safer_sk64_desc);
	TOMCRYPT_ADD_CIPHER(SAFER128, safer_sk128_desc);
#endif
#ifdef LTC_RIJNDAEL
	TOMCRYPT_ADD_CIPHER(RIJNDAEL, rijndael_desc);
	TOMCRYPT_ADD_CIPHER(AES, aes_desc);
/*	TOMCRYPT_ADD_CIPHER(RIJNDAEL_128, ...);*/
/*	TOMCRYPT_ADD_CIPHER(RIJNDAEL_192, ...);*/
/*	TOMCRYPT_ADD_CIPHER(RIJNDAEL_256, ...);*/
#endif
#ifdef LTC_XTEA
	TOMCRYPT_ADD_CIPHER(XTEA, xtea_desc);
#endif
#ifdef LTC_TWOFISH
	TOMCRYPT_ADD_CIPHER(TWOFISH, twofish_desc);
#endif
#ifdef LTC_DES
	TOMCRYPT_ADD_CIPHER(DES, des_desc);
	TOMCRYPT_ADD_CIPHER(3DES, des3_desc);
	TOMCRYPT_ADD_CIPHER(TRIPLEDES, des3_desc);
#endif
#ifdef LTC_CAST5
	TOMCRYPT_ADD_CIPHER(CAST5, cast5_desc);
#endif
#ifdef LTC_NOEKEON
	TOMCRYPT_ADD_CIPHER(NOEKEON, noekeon_desc);
#endif
#ifdef LTC_SKIPJACK
	TOMCRYPT_ADD_CIPHER(SKIPJACK, skipjack_desc);
#endif
#ifdef LTC_KHAZAD
	TOMCRYPT_ADD_CIPHER(KHAZAD, khazad_desc);
#endif
#ifdef LTC_ANUBIS
	TOMCRYPT_ADD_CIPHER(ANUBIS, anubis_desc);
#endif
#ifdef LTC_KSEED
	TOMCRYPT_ADD_CIPHER(KSEED, kseed_desc);
#endif
#ifdef LTC_KASUMI
	TOMCRYPT_ADD_CIPHER(KASUMI, kasumi_desc);
#endif
#ifdef LTC_MULTI2
	TOMCRYPT_ADD_CIPHER(MULTI2, multi2_desc);
#endif


	/* Hashes */
#ifdef LTC_SHA512
	TOMCRYPT_ADD_HASH(SHA512, sha512_desc);
#endif
#ifdef LTC_SHA384
	TOMCRYPT_ADD_HASH(SHA384, sha384_desc);
#endif
#ifdef LTC_SHA256
	TOMCRYPT_ADD_HASH(SHA256, sha256_desc);
#endif
#ifdef LTC_SHA224
	TOMCRYPT_ADD_HASH(SHA224, sha224_desc);
#endif
#ifdef LTC_SHA1
	TOMCRYPT_ADD_HASH(SHA1, sha1_desc);
#endif
#ifdef LTC_MD5
	TOMCRYPT_ADD_HASH(MD5, md5_desc);
#endif
#ifdef LTC_MD4
	TOMCRYPT_ADD_HASH(MD4, md4_desc);
#endif
#ifdef LTC_TIGER
	TOMCRYPT_ADD_HASH(TIGER, tiger_desc);
#endif
#ifdef LTC_MD2
	TOMCRYPT_ADD_HASH(MD2, md2_desc);
#endif
#ifdef LTC_RIPEMD128
	TOMCRYPT_ADD_HASH(RIPEMD128, rmd128_desc);
#endif
#ifdef LTC_RIPEMD160
	TOMCRYPT_ADD_HASH(RIPEMD160, rmd160_desc);
#endif
#ifdef LTC_RIPEMD256
	TOMCRYPT_ADD_HASH(RIPEMD256, rmd256_desc);
#endif
#ifdef LTC_RIPEMD320
	TOMCRYPT_ADD_HASH(RIPEMD320, rmd320_desc);
#endif
#ifdef LTC_WHIRLPOOL
	TOMCRYPT_ADD_HASH(WHIRLPOOL, whirlpool_desc);
#endif
#ifdef LTC_CHC
	TOMCRYPT_ADD_HASH(CHC, chc_desc);
#endif


	/* MACs */
#ifdef LTC_HMAC
	TOMCRYPT_ADD_MAC(HMAC);
#endif
#ifdef LTC_OMAC
	TOMCRYPT_ADD_MAC(CMAC);
#endif
#ifdef LTC_PMAC
	TOMCRYPT_ADD_MAC(PMAC);
#endif
#ifdef LTC_PELICAN
	TOMCRYPT_ADD_MAC(PELICAN);
#endif
#ifdef LTC_XCBC
	TOMCRYPT_ADD_MAC(XCBC);
#endif


	/* PRNGs */
#ifdef LTC_YARROW
	TOMCRYPT_ADD_RNG(YARROW, yarrow_desc);
#endif
#ifdef LTC_RC4
	TOMCRYPT_ADD_RNG(RC4, rc4_desc);
#endif
#ifdef LTC_FORTUNA
	TOMCRYPT_ADD_RNG(FORTUNA, fortuna_desc);
#endif
#ifdef LTC_SOBER128
	TOMCRYPT_ADD_RNG(SOBER128, sober128_desc);
#endif
#ifdef LTC_SPRNG
	TOMCRYPT_ADD_RNG(SECURE, sprng_desc);
#endif

	for (i = 0; php_tomcrypt_rngs[i].name != NULL; i++) {
		rng_make_prng(128, find_prng(php_tomcrypt_rngs[i].name),
			&php_tomcrypt_rngs[i].state, NULL);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(tomcrypt)
{
	int i, index;

	for (i = 0; php_tomcrypt_rngs[i].name != NULL; i++) {
		index = find_prng(php_tomcrypt_rngs[i].name);
		prng_descriptor[index].done(&php_tomcrypt_rngs[i].state);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(tomcrypt)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "libtomcrypt support", "enabled");
	php_info_print_table_header(2, "extension version", PHP_TOMCRYPT_VERSION);
	php_info_print_table_header(2, "libtomcrypt header version", SCRYPT);
	php_info_print_table_header(2, "library configuration", crypt_build_settings);
	php_info_print_table_end();
}
/* }}} */


#include "ext/standard/php_smart_str.h"

/***************/
/*    MISC.    */
/***************/

/* {{{ proto string tomcrypt_strerror(int errno)
   Retrieve the error message for the given errno */
PHP_FUNCTION(tomcrypt_strerror)
{
	long   err;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
		&err) == FAILURE) {
		return;
	}

	RETVAL_STRING(error_to_string(err), 1);
}
/* }}} */

#ifdef LTC_BASE64
/* {{{ proto string tomcrypt_base64_encode(string data)
   Encode some data using the Base64-encoding scheme */
PHP_FUNCTION(tomcrypt_base64_encode)
{
	char          *data, *buf;
	int            data_len;
	unsigned long  out_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

	buf = data;
	if (base64_encode(data, data_len, buf, &out_len) != CRYPT_BUFFER_OVERFLOW) {
		RETURN_FALSE;
	}

	buf = emalloc(out_len);
	if (base64_encode(data, data_len, buf, &out_len) != CRYPT_OK) {
		efree(buf);
		RETURN_FALSE;
	}

	RETVAL_STRINGL(buf, out_len, 0);
}
/* }}} */

/* {{{ proto string tomcrypt_base64_decode(string data)
   Decode Base64-encoded data */
PHP_FUNCTION(tomcrypt_base64_decode)
{
	char          *data, *buf;
	int            data_len;
	unsigned long  out_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

	out_len = data_len * 3 / 4 + 1;
	buf = emalloc(out_len);
	if (base64_decode(data, data_len, buf, &out_len) != CRYPT_OK) {
		efree(buf);
		RETURN_FALSE;
	}

	buf[out_len] = '\0';
	RETVAL_STRINGL(buf, out_len, 0);
}
/* }}} */
#endif /* LTC_BASE64 */


/***************/
/*    LISTS    */
/***************/

/* {{{ proto array tomcrypt_list_modes()
   List all available cipher modes */
PHP_FUNCTION(tomcrypt_list_modes)
{
	int   i = 0;

	array_init(return_value);

#define APPEND_MODE(mode) \
	add_index_string(return_value, i++, PHP_TOMCRYPT_MODE_ ## mode, 1);

#ifdef LTC_ECB_MODE
	APPEND_MODE(ECB);
#endif
#ifdef LTC_CFB_MODE
	APPEND_MODE(CFB);
#endif
#ifdef LTC_OFB_MODE
	APPEND_MODE(OFB);
#endif
#ifdef LTC_CBC_MODE
	APPEND_MODE(CBC);
#endif
#ifdef LTC_CTR_MODE
	APPEND_MODE(CTR);
#endif
#ifdef LTC_LRW_MODE
	APPEND_MODE(LRW);
#endif
#ifdef LTC_F8_MODE
	APPEND_MODE(F8);
#endif
#ifdef LTC_XTS_MODE
	APPEND_MODE(XTS);
#endif
#ifdef LTC_CCM_MODE
	APPEND_MODE(CCM);
#endif
#ifdef LTC_GCM_MODE
	APPEND_MODE(GCM);
#endif
#ifdef LTC_EAX_MODE
	APPEND_MODE(EAX);
#endif
#ifdef LTC_OCB_MODE
	APPEND_MODE(OCB);
#endif
}
/* }}} */

/* {{{ proto array tomcrypt_list_ciphers()
   List all available ciphers */
PHP_FUNCTION(tomcrypt_list_ciphers)
{
	int   i;

	array_init(return_value);

	for (i = 0; cipher_descriptor[i].name != NULL; i++) {
		add_index_string(return_value, i, cipher_descriptor[i].name, 1);
	}
}
/* }}} */

/* {{{ proto array tomcrypt_list_hashes()
   List all available hash algorithms */
PHP_FUNCTION(tomcrypt_list_hashes)
{
	int   i;

	array_init(return_value);

	for (i = 0; hash_descriptor[i].name != NULL; i++) {
		add_index_string(return_value, i, hash_descriptor[i].name, 1);
	}
}
/* }}} */

/* {{{ proto array tomcrypt_list_macs()
   List all available MAC protocols */
PHP_FUNCTION(tomcrypt_list_macs)
{
	int   i = 0;

	array_init(return_value);

#define APPEND_MAC(mac) \
	add_index_string(return_value, i++, PHP_TOMCRYPT_MAC_ ## mac, 1);

#ifdef LTC_HMAC
	APPEND_MAC(HMAC);
#endif
#ifdef LTC_OMAC
	APPEND_MAC(CMAC);
#endif
#ifdef LTC_PMAC
	APPEND_MAC(PMAC);
#endif
#ifdef LTC_PELICAN
	APPEND_MAC(PELICAN);
#endif
#ifdef LTC_XCBC
	APPEND_MAC(XCBC);
#endif
}
/* }}} */

/* {{{ proto array tomcrypt_list_rngs()
   List all available (Pseudo-)Random Number Generators (PRNGs) */
PHP_FUNCTION(tomcrypt_list_rngs)
{
	int   i = 0;

	array_init(return_value);

#define APPEND_RNG(rng) \
	add_index_string(return_value, i++, PHP_TOMCRYPT_RNG_ ## rng, 1);

#ifdef LTC_YARROW
	APPEND_RNG(YARROW);
#endif
#ifdef LTC_RC4
	APPEND_RNG(RC4);
#endif
#ifdef LTC_FORTUNA
	APPEND_RNG(FORTUNA);
#endif
#ifdef LTC_SPRNG
	APPEND_RNG(SECURE);
#endif
#ifdef LTC_SOBER128
	APPEND_RNG(SOBER128);
#endif
}
/* }}} */


/*****************/
/*    CIPHERS    */
/*****************/

/* {{{ proto string tomcrypt_cipher_name(string cipher)
   Get the name of the specified cipher */
PHP_FUNCTION(tomcrypt_cipher_name)
{
	char *cipher;
	int   cipher_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&cipher, &cipher_len) == FAILURE) {
		return;
	}

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	RETVAL_STRING(cipher_descriptor[index].name, 1);
}
/* }}} */

/* {{{ proto int tomcrypt_cipher_block_size(string cipher)
   Get the block size of the specified cipher in bytes */
PHP_FUNCTION(tomcrypt_cipher_block_size)
{
	char *cipher;
	int   cipher_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&cipher, &cipher_len) == FAILURE) {
		return;
	}

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	RETVAL_LONG(cipher_descriptor[index].block_length);
}
/* }}} */

/* {{{ proto int tomcrypt_cipher_adapt_key_size(string cipher, int keysize)
   Derive an appropriate key size for the specified cipher */
PHP_FUNCTION(tomcrypt_cipher_adapt_key_size)
{
	char *cipher;
	int   cipher_len, err, index, size;
	long  keysize;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
		&cipher, &cipher_len, &keysize) == FAILURE) {
		return;
	}

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	/* Detect key sizes that would cause issues (eg. overflows). */
	size = (keysize < 0 || keysize >= 0xFFFFFFFF) ? 0 : (int) keysize;

	if ((err = cipher_descriptor[index].keysize(&size)) != CRYPT_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
		RETURN_FALSE;
	}

	RETVAL_LONG((long) size);
}
/* }}} */

/* {{{ proto int tomcrypt_cipher_min_key_size(string cipher)
   Get the minimum key size of the specified cipher in bytes */
PHP_FUNCTION(tomcrypt_cipher_min_key_size)
{
	char *cipher;
	int   cipher_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&cipher, &cipher_len) == FAILURE) {
		return;
	}

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	RETVAL_LONG(cipher_descriptor[index].min_key_length);
}
/* }}} */

/* {{{ proto int tomcrypt_cipher_max_key_size(string cipher)
   Get the maximum key size of the specified cipher in bytes */
PHP_FUNCTION(tomcrypt_cipher_max_key_size)
{
	char *cipher;
	int   cipher_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&cipher, &cipher_len) == FAILURE) {
		return;
	}

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	RETVAL_LONG(cipher_descriptor[index].max_key_length);
}
/* }}} */

/* {{{ proto int tomcrypt_cipher_default_rounds(string cipher)
   Get the default number of rounds for the specified cipher */
PHP_FUNCTION(tomcrypt_cipher_default_rounds)
{
	char *cipher;
	int   cipher_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&cipher, &cipher_len) == FAILURE) {
		return;
	}

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	RETVAL_LONG(cipher_descriptor[index].default_rounds);
}
/* }}} */

#define GET_OPT_STRING(arr, key, dest, destlen, defval) { \
	zval **item; \
	if (arr && zend_hash_find(Z_ARRVAL_P(arr), key, sizeof(key), (void**)&item) == SUCCESS && Z_TYPE_PP(item) == IS_STRING) { \
		dest = Z_STRVAL_PP(item); \
		destlen = Z_STRLEN_PP(item); \
	} else { \
		dest = defval; \
		destlen = (defval == NULL) ? 0 : sizeof(defval); \
	} \
}

#define GET_OPT_LONG(arr, key, dest, defval)	{ \
	zval **item; \
	if (arr && zend_hash_find(Z_ARRVAL_P(arr), key, sizeof(key), (void**)&item) == SUCCESS && Z_TYPE_PP(item) == IS_LONG) \
		dest = Z_LVAL_PP(item); \
	else \
		dest = defval; \
}

/* {{{ proto int tomcrypt_cipher_encrypt(string cipher, string key,
                                         string plaintext, string mode,
                                        &array options = array())
   Encrypt some data */
PHP_FUNCTION(tomcrypt_cipher_encrypt)
{
	char  *cipher, *key, *plaintext, *mode, *ciphertext = NULL;
	int    cipher_len, key_len, plaintext_len, mode_len;
	zval  *options = NULL;
	int    index, err, num_rounds = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssss|a!",
		&cipher, &cipher_len, &key, &key_len, &plaintext, &plaintext_len,
		&mode, &mode_len, &options) == FAILURE) {
		return;
	}
	GET_OPT_LONG(options, "rounds", num_rounds, 0);

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	ciphertext = emalloc(plaintext_len + 1);
	ciphertext[plaintext_len] = '\0';

	if (!strncmp(PHP_TOMCRYPT_MODE_ECB, mode, mode_len)) {
#ifdef LTC_ECB_MODE
		symmetric_ECB  ctx;
		if ((err = ecb_start(index, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = ecb_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = ecb_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CFB, mode, mode_len)) {
#ifdef LTC_CFB_MODE
		symmetric_CFB  ctx;
		char          *iv;
		int            iv_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = cfb_start(index, iv, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = cfb_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = cfb_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_OFB, mode, mode_len)) {
#ifdef LTC_OFB_MODE
		symmetric_OFB  ctx;
		char          *iv;
		int            iv_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = ofb_start(index, iv, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = ofb_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = ofb_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CBC, mode, mode_len)) {
#ifdef LTC_CBC_MODE
		symmetric_CBC  ctx;
		char          *iv;
		int            iv_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = cbc_start(index, iv, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = cbc_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = cbc_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CTR, mode, mode_len)) {
#ifdef LTC_CTR_MODE
		symmetric_CTR  ctx;
		char          *iv;
		int            iv_len, ctr_mode;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		GET_OPT_LONG(options, "ctr_mode", ctr_mode, 0);
		if ((err = ctr_start(index, iv, key, key_len, num_rounds, ctr_mode, &ctx)) != CRYPT_OK ||
			(err = ctr_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = ctr_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_LRW, mode, mode_len)) {
#ifdef LTC_LRW_MODE
		symmetric_LRW  ctx;
		char          *iv, *tweak;
		int            iv_len, tweak_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		GET_OPT_STRING(options, "tweak", tweak, tweak_len, NULL);
		/* LRW uses a fixed-length tweak value. */
		if (tweak_len != 16) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid tweak size (%d), expected %d",
				tweak_len, 16);
			RETURN_FALSE;
		}

		if ((err = lrw_start(index, iv, key, key_len, tweak, num_rounds, &ctx)) != CRYPT_OK ||
			(err = lrw_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = lrw_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_F8, mode, mode_len)) {
#ifdef LTC_F8_MODE
		symmetric_F8   ctx;
		char          *iv, *salt_key;
		int            iv_len, salt_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		GET_OPT_STRING(options, "salt_key", salt_key, salt_len, NULL);
		if (salt_key == NULL) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "A salt_key is required in F8 mode");
			RETURN_FALSE;
		}

		if ((err = f8_start(index, iv, key, key_len, salt_key, salt_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = f8_encrypt(plaintext, ciphertext, plaintext_len, &ctx)) != CRYPT_OK ||
			(err = f8_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_XTS, mode, mode_len)) {
#ifdef LTC_XTS_MODE
		symmetric_xts  ctx;
		char          *key2, *tweak;
		int            key2_len, tweak_len;

		GET_OPT_STRING(options, "key2", key2, key2_len, NULL);
		if (key2_len != key_len) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid length for key2 (%d), expected %d",
				key2_len, key_len);
			RETURN_FALSE;
		}

		GET_OPT_STRING(options, "tweak", tweak, tweak_len, NULL);
		/* XTS uses a fixed-length tweak value. */
		if (tweak_len != 16) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid tweak size (%d), expected %d",
				tweak_len, 16);
			RETURN_FALSE;
		}

		/* API inconsistency: xts_encrypt() changes the usual order
		 *                    of XXX_encrypt() arguments. */
		if ((err = xts_start(index, key, key2, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = xts_encrypt(plaintext, plaintext_len, ciphertext, tweak, &ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		xts_done(&ctx);
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CCM, mode, mode_len)) {
#ifdef LTC_CCM_MODE
		char          *nonce, *authdata, tag[MAXBLOCKSIZE + 1];
		int            nonce_len, authdata_len;
		unsigned long  tag_len;

		GET_OPT_STRING(options, "nonce", nonce, nonce_len, NULL);
		GET_OPT_STRING(options, "authdata", authdata, authdata_len, NULL);

		if ((err = ccm_memory(index, key, key_len, NULL, nonce, nonce_len, authdata, authdata_len,
			plaintext, plaintext_len, ciphertext, tag, &tag_len, CCM_ENCRYPT)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		/* Write the tag back. */
		if (options) {
			tag[tag_len] = '\0';
			add_assoc_stringl(options, "tag", tag, tag_len, 1);
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_GCM, mode, mode_len)) {
#ifdef LTC_GCM_MODE
		char          *iv, *authdata, tag[MAXBLOCKSIZE + 1];
		int            iv_len, authdata_len;
		unsigned long  tag_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		GET_OPT_STRING(options, "authdata", authdata, authdata_len, NULL);

		if ((err = gcm_memory(index, key, key_len, iv, iv_len, authdata, authdata_len,
			plaintext, plaintext_len, ciphertext, tag, &tag_len, GCM_ENCRYPT)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		/* Write the tag back. */
		if (options) {
			tag[tag_len] = '\0';
			add_assoc_stringl(options, "tag", tag, tag_len, 1);
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_EAX, mode, mode_len)) {
#ifdef LTC_EAX_MODE
		char          *nonce, *authdata, tag[MAXBLOCKSIZE + 1];
		int            nonce_len, authdata_len;
		unsigned long  tag_len;

		GET_OPT_STRING(options, "nonce", nonce, nonce_len, NULL);
		GET_OPT_STRING(options, "authdata", authdata, authdata_len, NULL);

		if ((err = eax_encrypt_authenticate_memory(
			index, key, key_len, nonce, nonce_len, authdata, authdata_len,
			plaintext, plaintext_len, ciphertext, tag, &tag_len)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		/* Write the tag back. */
		if (options) {
			tag[tag_len] = '\0';
			add_assoc_stringl(options, "tag", tag, tag_len, 1);
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_OCB, mode, mode_len)) {
#ifdef LTC_OCB_MODE
		char          *nonce, tag[MAXBLOCKSIZE + 1];
		int            nonce_len;
		unsigned long  tag_len;

		GET_OPT_STRING(options, "nonce", nonce, nonce_len, NULL);
		if (nonce_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid nonce size (%d), expected %d",
				nonce_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = ocb_encrypt_authenticate_memory(index, key, key_len, nonce,
			plaintext, plaintext_len, ciphertext, tag, &tag_len)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		/* Write the tag back. */
		if (options) {
			tag[tag_len] = '\0';
			add_assoc_stringl(options, "tag", tag, tag_len, 1);
		}
		RETURN_STRINGL(ciphertext, plaintext_len, 0);
#endif
	}

	/* Unsupported mode (invalid name, not compiled, etc.) */
	efree(ciphertext);
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unsupported mode: %s", mode);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto int tomcrypt_cipher_decrypt(string cipher, string key,
                                         string ciphertext, string mode,
                                         array options = array())
   Encrypt some data */
PHP_FUNCTION(tomcrypt_cipher_decrypt)
{
	char  *cipher, *key, *plaintext = NULL, *mode, *ciphertext;
	int    cipher_len, key_len, ciphertext_len, mode_len;
	zval  *options = NULL;
	int    index, err, num_rounds = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssss|a!",
		&cipher, &cipher_len, &key, &key_len, &ciphertext, &ciphertext_len,
		&mode, &mode_len, &options) == FAILURE) {
		return;
	}
	GET_OPT_LONG(options, "rounds", num_rounds, 0);

	if ((index = find_cipher(cipher)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown cipher: %s", cipher);
		RETURN_FALSE;
	}

	plaintext = emalloc(ciphertext_len + 1);
	plaintext[ciphertext_len] = '\0';

	if (!strncmp(PHP_TOMCRYPT_MODE_ECB, mode, mode_len)) {
#ifdef LTC_ECB_MODE
		symmetric_ECB  ctx;
		if ((err = ecb_start(index, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = ecb_encrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = ecb_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CFB, mode, mode_len)) {
#ifdef LTC_CFB_MODE
		symmetric_CFB  ctx;
		char          *iv;
		int            iv_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = cfb_start(index, iv, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = cfb_decrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = cfb_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_OFB, mode, mode_len)) {
#ifdef LTC_OFB_MODE
		symmetric_OFB  ctx;
		char          *iv;
		int            iv_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = ofb_start(index, iv, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = ofb_decrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = ofb_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CBC, mode, mode_len)) {
#ifdef LTC_CBC_MODE
		symmetric_CBC  ctx;
		char          *iv;
		int            iv_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = cbc_start(index, iv, key, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = cbc_decrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = cbc_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CTR, mode, mode_len)) {
#ifdef LTC_CTR_MODE
		symmetric_CTR  ctx;
		char          *iv;
		int            iv_len, ctr_mode;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		GET_OPT_LONG(options, "ctr_mode", ctr_mode, 0);
		if ((err = ctr_start(index, iv, key, key_len, num_rounds, ctr_mode, &ctx)) != CRYPT_OK ||
			(err = ctr_decrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = ctr_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_LRW, mode, mode_len)) {
#ifdef LTC_LRW_MODE
		symmetric_LRW  ctx;
		char          *iv, *tweak;
		int            iv_len, tweak_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		GET_OPT_STRING(options, "tweak", tweak, tweak_len, NULL);
		/* LRW uses a fixed-length tweak value. */
		if (tweak_len != 16) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid tweak size (%d), expected %d",
				tweak_len, 16);
			RETURN_FALSE;
		}

		if ((err = lrw_start(index, iv, key, key_len, tweak, num_rounds, &ctx)) != CRYPT_OK ||
			(err = lrw_decrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = lrw_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_F8, mode, mode_len)) {
#ifdef LTC_F8_MODE
		symmetric_F8   ctx;
		char          *iv, *salt_key;
		int            iv_len, salt_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		if (iv_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d",
				iv_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		GET_OPT_STRING(options, "salt_key", salt_key, salt_len, NULL);
		if (salt_key == NULL) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "A salt_key is required in F8 mode");
			RETURN_FALSE;
		}

		if ((err = f8_start(index, iv, key, key_len, salt_key, salt_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = f8_decrypt(ciphertext, plaintext, ciphertext_len, &ctx)) != CRYPT_OK ||
			(err = f8_done(&ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_XTS, mode, mode_len)) {
#ifdef LTC_XTS_MODE
		symmetric_xts  ctx;
		char          *key2, *tweak;
		int            key2_len, tweak_len;

		GET_OPT_STRING(options, "key2", key2, key2_len, NULL);
		if (key2_len != key_len) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid length for key2 (%d), expected %d",
				key2_len, key_len);
			RETURN_FALSE;
		}

		GET_OPT_STRING(options, "tweak", tweak, tweak_len, NULL);
		/* XTS uses a fixed-length tweak value. */
		if (tweak_len != 16) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid tweak size (%d), expected %d",
				tweak_len, 16);
			RETURN_FALSE;
		}

		/* API inconsistency: xts_encrypt() changes the usual order
		 *                    of XXX_encrypt() arguments. */
		if ((err = xts_start(index, key, key2, key_len, num_rounds, &ctx)) != CRYPT_OK ||
			(err = xts_decrypt(ciphertext, ciphertext_len, plaintext, tweak, &ctx)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
		xts_done(&ctx);
		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_CCM, mode, mode_len)) {
#ifdef LTC_CCM_MODE
		char          *nonce, *authdata, *tag, tag2[MAXBLOCKSIZE + 1];
		int            nonce_len, authdata_len, tag_len;
		unsigned long  tag2_len;

		GET_OPT_STRING(options, "nonce", nonce, nonce_len, NULL);
		GET_OPT_STRING(options, "authdata", authdata, authdata_len, NULL);
		GET_OPT_STRING(options, "tag", tag, tag_len, NULL);

		if ((err = ccm_memory(index, key, key_len, NULL, nonce, nonce_len, authdata, authdata_len,
			plaintext, ciphertext_len, ciphertext, tag2, &tag2_len, CCM_DECRYPT)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		if (tag2_len != tag_len || memcmp(tag, tag2, tag_len)) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tag verification failed");
			RETURN_FALSE;
		}

		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_GCM, mode, mode_len)) {
#ifdef LTC_GCM_MODE
		char          *iv, *authdata, *tag, tag2[MAXBLOCKSIZE + 1];
		int            iv_len, authdata_len, tag_len;
		unsigned long  tag2_len;

		GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
		GET_OPT_STRING(options, "authdata", authdata, authdata_len, NULL);
		GET_OPT_STRING(options, "tag", tag, tag_len, NULL);

		if ((err = gcm_memory(index, key, key_len, iv, iv_len, authdata, authdata_len,
			plaintext, ciphertext_len, ciphertext, tag2, &tag2_len, GCM_DECRYPT)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		if (tag2_len != tag_len || memcmp(tag, tag2, tag_len)) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tag verification failed");
			RETURN_FALSE;
		}

		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_EAX, mode, mode_len)) {
#ifdef LTC_EAX_MODE
		char          *nonce, *authdata, *tag;
		int            nonce_len, authdata_len, tag_len, res;

		GET_OPT_STRING(options, "nonce", nonce, nonce_len, NULL);
		GET_OPT_STRING(options, "authdata", authdata, authdata_len, NULL);
		GET_OPT_STRING(options, "tag", tag, tag_len, NULL);

		if ((err = eax_decrypt_verify_memory(
			index, key, key_len, nonce, nonce_len, authdata, authdata_len,
			ciphertext, ciphertext_len, plaintext,
			tag, (unsigned long) tag_len, &res)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		/* Tag verification failed. */
		if (res != 1) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tag verification failed");
			RETURN_FALSE;
		}

		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	} else if (!strncmp(PHP_TOMCRYPT_MODE_OCB, mode, mode_len)) {
#ifdef LTC_OCB_MODE
		char          *nonce, *tag;
		int            nonce_len, tag_len, res;

		GET_OPT_STRING(options, "nonce", nonce, nonce_len, NULL);
		if (nonce_len != cipher_descriptor[index].block_length) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid nonce size (%d), expected %d",
				nonce_len, cipher_descriptor[index].block_length);
			RETURN_FALSE;
		}

		if ((err = ocb_decrypt_verify_memory(index, key, key_len, nonce,
			ciphertext, ciphertext_len, plaintext,
			tag, (unsigned long) tag_len, &res)) != CRYPT_OK) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}

		/* Tag verification failed. */
		if (res != 1) {
			efree(ciphertext);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Tag verification failed");
			RETURN_FALSE;
		}

		RETURN_STRINGL(plaintext, ciphertext_len, 0);
#endif
	}

	/* Unsupported mode (invalid name, not compiled, etc.) */
	efree(ciphertext);
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unsupported mode: %s", mode);
	RETURN_FALSE;
}
/* }}} */


/****************/
/*    HASHES    */
/****************/

static void php_tomcrypt_do_hash(INTERNAL_FUNCTION_PARAMETERS, int isfilename, zend_bool raw_output_default) /* {{{ */
{
	char       *algo, *data, hash[MAXBLOCKSIZE + 1];
	int         algo_len, data_len;
	int         index, err;
	hash_state  md;
	zend_bool   raw_output = raw_output_default;
	php_stream *stream = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|b",
		&algo, &algo_len, &data, &data_len, &raw_output) == FAILURE) {
		return;
	}

	if ((index = find_hash(algo)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}

	hash_descriptor[index].init(&md);
	if (isfilename) {
		char buf[1024];
		long n;

		if (CHECK_NULL_PATH(data, data_len)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid path");
			RETURN_FALSE;
		}
		stream = php_stream_open_wrapper_ex(data, "rb", REPORT_ERRORS, NULL, DEFAULT_CONTEXT);
		if (!stream) {
			/* Stream will report errors opening file */
			RETURN_FALSE;
		}

		while ((n = php_stream_read(stream, buf, sizeof(buf))) > 0) {
			if ((err = hash_descriptor[index].process(&md, (unsigned char *) buf, n)) != CRYPT_OK) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
				RETURN_FALSE;
			}
		}
		php_stream_close(stream);
	} else {
		if ((err = hash_descriptor[index].process(&md, data, data_len)) != CRYPT_OK) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
	}

	hash_descriptor[index].done(&md, hash);
	hash[hash_descriptor[index].hashsize] = '\0';

	if (raw_output) {
		RETURN_STRINGL(hash, hash_descriptor[index].hashsize, 1);
	} else {
		char *hex_digest = safe_emalloc(hash_descriptor[index].hashsize, 2, 1);

		php_tomcrypt_bin2hex(hex_digest, (unsigned char *) hash, hash_descriptor[index].hashsize);
		hex_digest[2 * hash_descriptor[index].hashsize] = '\0';
		RETURN_STRINGL(hex_digest, 2 * hash_descriptor[index].hashsize, 0);
	}
}
/* }}} */

/* {{{ proto string tomcrypt_hash_name(string hash)
   Get the name of the specified hashing algorithm */
PHP_FUNCTION(tomcrypt_hash_name)
{
	char *algo;
	int   algo_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&algo, &algo_len) == FAILURE) {
		return;
	}

	if ((index = find_hash(algo)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}

	RETVAL_STRING(hash_descriptor[index].name, 1);
}
/* }}} */

/* {{{ proto int tomcrypt_hash_block_size(string hash)
   Get the block size of the specified hashing algorithm in bytes */
PHP_FUNCTION(tomcrypt_hash_block_size)
{
	char *algo;
	int   algo_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&algo, &algo_len) == FAILURE) {
		return;
	}

	if ((index = find_hash(algo)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}

	RETVAL_LONG(hash_descriptor[index].blocksize);
}
/* }}} */

/* {{{ proto int tomcrypt_hash_digest_size(string hash)
   Get the digest output size of the specified hashing algorithm in bytes */
PHP_FUNCTION(tomcrypt_hash_digest_size)
{
	char *algo;
	int   algo_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&algo, &algo_len) == FAILURE) {
		return;
	}

	if ((index = find_hash(algo)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}

	RETVAL_LONG(hash_descriptor[index].hashsize);
}
/* }}} */

/* {{{ proto int tomcrypt_hash_string(string algo, string data, bool raw_output = false)
   Compute the hash of a string using the specified algorithm */
PHP_FUNCTION(tomcrypt_hash_string)
{
	php_tomcrypt_do_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_hash_file(string algo, string filename, bool raw_output = false)
   Compute the hash of a file using the specified algorithm */
PHP_FUNCTION(tomcrypt_hash_file)
{
	php_tomcrypt_do_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, 0);
}
/* }}} */


/**************/
/*    xMAC    */
/**************/

typedef int (*php_tomcrypt_mac_finder)(const char *name);
typedef int (*php_tomcrypt_mac_init)(void *state, int algo, const unsigned char *key, unsigned long keylen);
typedef int (*php_tomcrypt_mac_process)(void *state, const unsigned char *in, unsigned long inlen);
typedef int (*php_tomcrypt_mac_done)(void *state, unsigned char *out, unsigned long *outlen);

typedef struct _php_tomcrypt_mac {
	php_tomcrypt_mac_finder   finder;
	php_tomcrypt_mac_init     init;
	php_tomcrypt_mac_process  process;
	php_tomcrypt_mac_done     done;
	void                     *state;
} php_tomcrypt_mac;

static void php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAMETERS, php_tomcrypt_mac *desc,
                                int isfilename, zend_bool raw_output_default) /* {{{ */
{
	char           *algo, *key, *data, mac[MAXBLOCKSIZE + 1];
	unsigned long   macsize = MAXBLOCKSIZE;
	int             algo_len, key_len, data_len;
	int             err, index;
	zend_bool       raw_output = raw_output_default;
	php_stream     *stream = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|b",
		&algo, &algo_len, &key, &key_len, &data, &data_len, &raw_output) == FAILURE) {
		return;
	}

	if ((index = desc->finder(algo)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown algorithm: %s", algo);
		RETURN_FALSE;
	}

	if ((err = desc->init(&desc->state, index, key, key_len)) != CRYPT_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
		RETURN_FALSE;
	}

	if (isfilename) {
		char buf[1024];
		long n;

		if (CHECK_NULL_PATH(data, data_len)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid path");
			RETURN_FALSE;
		}
		stream = php_stream_open_wrapper_ex(data, "rb", REPORT_ERRORS, NULL, DEFAULT_CONTEXT);
		if (!stream) {
			/* Stream will report errors opening file */
			RETURN_FALSE;
		}

		while ((n = php_stream_read(stream, buf, sizeof(buf))) > 0) {
			if ((err = desc->process(&desc->state, (unsigned char *) buf, n)) != CRYPT_OK) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
				RETURN_FALSE;
			}
		}
		php_stream_close(stream);
	} else {
		if ((err = desc->process(&desc->state, data, data_len)) != CRYPT_OK) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
			RETURN_FALSE;
		}
	}

	if ((err = desc->done(&desc->state, mac, &macsize)) != CRYPT_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
		RETURN_FALSE;
	}

	mac[macsize] = '\0';

	if (raw_output) {
		RETURN_STRINGL(mac, macsize, 1);
	} else {
		char *hex_digest = safe_emalloc(macsize, 2, 1);

		php_tomcrypt_bin2hex(hex_digest, (unsigned char *) mac, macsize);
		hex_digest[2 * macsize] = '\0';
		RETURN_STRINGL(hex_digest, 2 * macsize, 0);
	}
}
/* }}} */

#ifdef LTC_HMAC
/* {{{ proto int tomcrypt_hmac_string(string hash, string key, string data, bool raw_output = false)
   Compute the HMAC of a string using the specified hashing algorithm */
PHP_FUNCTION(tomcrypt_hmac_string)
{
	hmac_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_hash,
		(php_tomcrypt_mac_init)    hmac_init,
		(php_tomcrypt_mac_process) hmac_process,
		(php_tomcrypt_mac_done)    hmac_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_hmac_file(string hash, string key, string filename, bool raw_output = false)
   Compute the HMAC of a file using the specified hashing algorithm */
PHP_FUNCTION(tomcrypt_hmac_file)
{
	hmac_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_hash,
		(php_tomcrypt_mac_init)    hmac_init,
		(php_tomcrypt_mac_process) hmac_process,
		(php_tomcrypt_mac_done)    hmac_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 1, 0);
}
/* }}} */
#endif /* LTC_HMAC */

#ifdef LTC_OMAC
/* {{{ proto int tomcrypt_cmac_string(string cipher, string key, string data, bool raw_output = false)
   Compute the CMAC (aka. OMAC) of a string using the specified cipher */
PHP_FUNCTION(tomcrypt_cmac_string)
{
	omac_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    omac_init,
		(php_tomcrypt_mac_process) omac_process,
		(php_tomcrypt_mac_done)    omac_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_cmac_file(string cipher, string key, string filename, bool raw_output = false)
   Compute the CMAC (aka. OMAC) of a file using the specified cipher */
PHP_FUNCTION(tomcrypt_cmac_file)
{
	omac_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    omac_init,
		(php_tomcrypt_mac_process) omac_process,
		(php_tomcrypt_mac_done)    omac_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 1, 0);
}
/* }}} */
#endif /* LTC_OMAC */

#ifdef LTC_PMAC
/* {{{ proto int tomcrypt_pmac_string(string cipher, string key, string data, bool raw_output = false)
   Compute the PMAC of a string using the specified cipher */
PHP_FUNCTION(tomcrypt_pmac_string)
{
	pmac_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    pmac_init,
		(php_tomcrypt_mac_process) pmac_process,
		(php_tomcrypt_mac_done)    pmac_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_pmac_file(string cipher, string key, string filename, bool raw_output = false)
   Compute the PMAC of a file using the specified cipher */
PHP_FUNCTION(tomcrypt_pmac_file)
{
	pmac_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    pmac_init,
		(php_tomcrypt_mac_process) pmac_process,
		(php_tomcrypt_mac_done)    pmac_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 1, 0);
}
/* }}} */
#endif /* LTC_PMAC */

#ifdef LTC_PELICAN
/* {{{ proto int tomcrypt_pelican_string(string cipher, string key, string data, bool raw_output = false)
   Compute the Pelican MAC of a string using the specified cipher */
PHP_FUNCTION(tomcrypt_pelican_string)
{
	pelican_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    pelican_init,
		(php_tomcrypt_mac_process) pelican_process,
		(php_tomcrypt_mac_done)    pelican_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_pelican_file(string cipher, string key, string filename, bool raw_output = false)
   Compute the Pelican MAC of a file using the specified cipher */
PHP_FUNCTION(tomcrypt_pelican_file)
{
	pelican_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    pelican_init,
		(php_tomcrypt_mac_process) pelican_process,
		(php_tomcrypt_mac_done)    pelican_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 1, 0);
}
/* }}} */
#endif /* LTC_PELICAN */

#ifdef LTC_XCBC
/* {{{ proto int tomcrypt_xcbc_string(string cipher, string key, string data, bool raw_output = false)
   Compute the XCBC-MAC of a string using the specified cipher */
PHP_FUNCTION(tomcrypt_xcbc_string)
{
	xcbc_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    xcbc_init,
		(php_tomcrypt_mac_process) xcbc_process,
		(php_tomcrypt_mac_done)    xcbc_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_xcbc_file(string cipher, string key, string filename, bool raw_output = false)
   Compute the XCBC MAC of a file using the specified cipher */
PHP_FUNCTION(tomcrypt_xcbc_file)
{
	xcbc_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    xcbc_init,
		(php_tomcrypt_mac_process) xcbc_process,
		(php_tomcrypt_mac_done)    xcbc_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 1, 0);
}
/* }}} */
#endif /* LTC_XCBC */

#ifdef LTC_F9_MODE
/* {{{ proto int tomcrypt_f9_string(string cipher, string key, string data, bool raw_output = false)
   Compute the F9 MAC of a string using the specified cipher */
PHP_FUNCTION(tomcrypt_f9_string)
{
	f9_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    f9_init,
		(php_tomcrypt_mac_process) f9_process,
		(php_tomcrypt_mac_done)    f9_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 0, 0);
}
/* }}} */

/* {{{ proto int tomcrypt_f9_file(string cipher, string key, string filename, bool raw_output = false)
   Compute the F9 MAC of a file using the specified cipher */
PHP_FUNCTION(tomcrypt_f9_file)
{
	f9_state state;
	php_tomcrypt_mac desc = {
		(php_tomcrypt_mac_finder)  find_cipher,
		(php_tomcrypt_mac_init)    f9_init,
		(php_tomcrypt_mac_process) f9_process,
		(php_tomcrypt_mac_done)    f9_done,
		&state
	};
	php_tomcrypt_do_mac(INTERNAL_FUNCTION_PARAM_PASSTHRU, &desc, 1, 0);
}
/* }}} */
#endif /* LTC_F9 */


/**************/
/*    RNGs    */
/**************/

/* {{{ proto string tomcrypt_rng_name(string rng)
   Get the name of the specified (Pseudo-)RNG */
PHP_FUNCTION(tomcrypt_rng_name)
{
	char *rng;
	int   rng_len, index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&rng, &rng_len) == FAILURE) {
		return;
	}

	if ((index = find_prng(rng)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown RNG: %s", rng);
		RETURN_FALSE;
	}

	RETVAL_STRING(prng_descriptor[index].name, 1);
}
/* }}} */

/* {{{ proto string tomcrypt_rng_get_bytes(int size, string rng = TOMCRYPT_RNG_SECURE)
   Get some random bytes from the (Pseudo-)RNG */
PHP_FUNCTION(tomcrypt_rng_get_bytes)
{
	char           sprng[] = "sprng";
	char          *rng = sprng;
	int            rng_len = sizeof("sprng"), index, index2, size;
	unsigned char *buffer;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|s",
		&size, &rng, &rng_len) == FAILURE) {
		return;
	}

	if (size <= 0) {
		RETURN_FALSE;
	}

	if ((index = find_prng(rng)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown RNG: %s", rng);
		RETURN_FALSE;
	}

	for (index2 = 0; php_tomcrypt_rngs[index2].name != NULL; index2++) {
		if (!strncmp(php_tomcrypt_rngs[index2].name, rng, rng_len)) {
			break;
		}
	}

	if (php_tomcrypt_rngs[index2].name == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown RNG: %s", rng);
		RETURN_FALSE;
	}

	if (prng_descriptor[index].ready(&php_tomcrypt_rngs[index2].state) != CRYPT_OK) {
		RETURN_FALSE;
	}

	buffer = emalloc(size + 1);
	size = (int) prng_descriptor[index].read(buffer, size,
		&php_tomcrypt_rngs[index2].state);
	buffer[size] = '\0';
	RETVAL_STRINGL(buffer, size, 0);
}
/* }}} */

#endif /* HAVE_LIBTOMCRYPT */