--TEST--
tomcrypt - SKIPJACK cipher
--SKIPIF--
<?php
    if (!extension_loaded("tomcrypt")) print "skip extension not loaded";
    if (!defined('TOMCRYPT_CIPHER_SKIPJACK')) print "skip cipher not available";
    if (!defined('TOMCRYPT_MODE_ECB')) print "skip mode not available";
?>
--FILE--
<?php
    $cipher = TOMCRYPT_CIPHER_SKIPJACK;
    var_dump(
        in_array($cipher, tomcrypt_list_ciphers()),
        tomcrypt_cipher_name($cipher),
        tomcrypt_cipher_block_size($cipher),
        tomcrypt_cipher_min_key_size($cipher),
        tomcrypt_cipher_max_key_size($cipher),
        tomcrypt_cipher_default_rounds($cipher)
    );

    // See http://csrc.nist.gov/publications/nistpubs/800-17/800-17.pdf
    // Note: libtomcrypt assumes little-endian values.
    $pt     = "\x80\x00\x00\x00\x00\x00\x00\x00";
    $key    = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    $ct     = tomcrypt_cipher_encrypt($cipher, $key, $pt, TOMCRYPT_MODE_ECB);
    var_dump(bin2hex($ct));

    $pt2    = tomcrypt_cipher_decrypt($cipher, $key, $ct, TOMCRYPT_MODE_ECB);
    var_dump($pt === $pt2);
?>
--EXPECT--
bool(true)
string(8) "skipjack"
int(8)
int(10)
int(10)
int(32)
string(16) "d7e30b5b8d2218d5"
bool(true)
