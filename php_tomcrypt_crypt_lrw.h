#include <tomcrypt.h>
#include "php_tomcrypt_compat.h"
#include "php_tomcrypt_crypt.h"

static void php_tomcrypt_xcrypt_lrw(PLTC_CRYPT_PARAM)
{
#ifdef LTC_LRW_MODE
	symmetric_LRW  ctx;
	char          *output, *iv, *tweak;
	int            err, iv_len, tweak_len, num_rounds;

	GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
	GET_OPT_LONG(options, "rounds", num_rounds, 0);
	GET_OPT_STRING(options, "tweak", tweak, tweak_len, NULL);

	output = emalloc(input_len + 1);
	output[input_len] = '\0';

	if (iv_len != cipher_descriptor[cipher].block_length) {
		efree(output);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d", iv_len, cipher_descriptor[cipher].block_length);
		RETURN_FALSE;
	}

	/* LRW uses a fixed-length tweak value. */
	if (tweak_len != 16) {
		efree(output);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid tweak size (%d), expected %d", tweak_len, 16);
		RETURN_FALSE;
	}

	if ((err = lrw_start(cipher, iv, key, key_len, tweak, num_rounds, &ctx)) != CRYPT_OK) {
	    goto error;
	}

    if (direction == PLTC_ENCRYPT) {
        err = lrw_encrypt(input, output, input_len, &ctx);
    } else {
        err = lrw_decrypt(input, output, input_len, &ctx);
    }

	if (err != CRYPT_OK) {
		goto error;
	}

	if ((err = lrw_done(&ctx)) != CRYPT_OK) {
		goto error;
	}
	PLTC_RETURN_STRINGL(output, input_len, 0);

error:
	efree(output);
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
	RETURN_FALSE;
#endif
}

