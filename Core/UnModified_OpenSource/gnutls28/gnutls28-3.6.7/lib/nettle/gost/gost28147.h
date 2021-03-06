/* gost28147.h

   The GOST 28147-89 (MAGMA) cipher function, described in RFC 5831.

   Copyright (C) 2015 Dmitry Eremin-Solenikov
   Copyright (C) 2012 Nikos Mavrogiannopoulos, Niels Möller

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see https://www.gnu.org/licenses/.
*/

#ifndef NETTLE_GOST28147_H_INCLUDED
#define NETTLE_GOST28147_H_INCLUDED

#include <nettle/nettle-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* S-Boxes & parameters */
#define gost28147_param_test_3411 _gnutls_gost28147_param_test_3411
#define gost28147_param_CryptoPro_3411 _gnutls_gost28147_param_CryptoPro_3411
#define gost28147_param_Test_89 _gnutls_gost28147_param_Test_89
#define gost28147_param_CryptoPro_A _gnutls_gost28147_param_CryptoPro_A
#define gost28147_param_CryptoPro_B _gnutls_gost28147_param_CryptoPro_B
#define gost28147_param_CryptoPro_C _gnutls_gost28147_param_CryptoPro_C
#define gost28147_param_CryptoPro_D _gnutls_gost28147_param_CryptoPro_D
#define gost28147_param_TC26_Z _gnutls_gost28147_param_TC26_Z

/* Private */
#define gost28147_encrypt_simple _gnutls_gost28147_encrypt_simple

/* Public functions */
#define gost28147_set_key _gnutls_gost28147_set_key
#define gost28147_set_param _gnutls_gost28147_set_param
#define gost28147_encrypt _gnutls_gost28147_encrypt
#define gost28147_encrypt_for_cfb _gnutls_gost28147_encrypt_for_cfb
#define gost28147_decrypt _gnutls_gost28147_decrypt

#define GOST28147_KEY_SIZE 32
#define GOST28147_BLOCK_SIZE 8

struct gost28147_ctx
{
  uint32_t key[GOST28147_KEY_SIZE/4];
  const uint32_t *sbox;
  int key_meshing;
  int key_count; /* Used for key meshing */
};

struct gost28147_param
{
  int key_meshing;
  uint32_t sbox[4*256];
};

extern const struct gost28147_param gost28147_param_test_3411;
extern const struct gost28147_param gost28147_param_CryptoPro_3411;
extern const struct gost28147_param gost28147_param_Test_89;
extern const struct gost28147_param gost28147_param_CryptoPro_A;
extern const struct gost28147_param gost28147_param_CryptoPro_B;
extern const struct gost28147_param gost28147_param_CryptoPro_C;
extern const struct gost28147_param gost28147_param_CryptoPro_D;
extern const struct gost28147_param gost28147_param_TC26_Z;

/* Internal interface for use by GOST R 34.11-94 */
void gost28147_encrypt_simple (const uint32_t *key, const uint32_t *sbox,
                               const uint32_t *in, uint32_t *out);

void
gost28147_set_key(struct gost28147_ctx *ctx, const uint8_t *key);

void
gost28147_set_param(struct gost28147_ctx *ctx,
		    const struct gost28147_param *param);

void
gost28147_encrypt(const struct gost28147_ctx *ctx,
		  size_t length, uint8_t *dst,
		  const uint8_t *src);
void
gost28147_decrypt(const struct gost28147_ctx *ctx,
		  size_t length, uint8_t *dst,
		  const uint8_t *src);
void
gost28147_encrypt_for_cfb(struct gost28147_ctx *ctx,
			  size_t length, uint8_t *dst,
			  const uint8_t *src);

#ifdef __cplusplus
}
#endif

#endif /* NETTLE_GOST28147_H_INCLUDED */
