#include <tomcrypt.h>
#include "php_tomcrypt_compat.h"
#include "php_tomcrypt_crypt.h"

static void php_tomcrypt_xcrypt_f8(PLTC_CRYPT_PARAM)
{
#ifdef LTC_F8_MODE
	symmetric_F8   ctx;
	char          *output, *iv, *salt_key;
	int            err, num_rounds, iv_len, salt_len;

	GET_OPT_STRING(options, "iv", iv, iv_len, NULL);
	GET_OPT_STRING(options, "salt_key", salt_key, salt_len, NULL);
	GET_OPT_LONG(options, "rounds", num_rounds, 0);

	output = emalloc(input_len + 1);
	output[input_len] = '\0';

	if (iv_len != cipher_descriptor[cipher].block_length) {
		efree(output);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid iv size (%d), expected %d", iv_len, cipher_descriptor[cipher].block_length);
		RETURN_FALSE;
	}

	if (salt_key == NULL) {
		efree(output);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "A salt_key is required in F8 mode");
		RETURN_FALSE;
	}


	if ((err = f8_start(cipher, iv, key, key_len, salt_key, salt_len, num_rounds, &ctx)) != CRYPT_OK) {
		goto error;
	}

	if (direction == PLTC_ENCRYPT) {
		err = f8_encrypt(input, output, input_len, &ctx);
	} else {
		err = f8_decrypt(input, output, input_len, &ctx);
	}

	if (err != CRYPT_OK) {
		goto error;
	}

	if ((err = f8_done(&ctx)) != CRYPT_OK) {
		goto error;
	}
	PLTC_RETURN_STRINGL(output, input_len, 0);

error:
	efree(output);
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error_to_string(err));
	RETURN_FALSE;
#endif
}

