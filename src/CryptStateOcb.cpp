/* Copyright (C) 2005-2011, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * This code implements OCB-AES128.
 * In the US, OCB is covered by patents. The inventor has given a license
 * to all programs distributed under the GPL.
 * Mumble is BSD (revised) licensed, meaning you can use the code in a
 * closed-source program. If you do, you'll have to either replace
 * OCB with something else or get yourself a license.
 */

#include "CryptStateOcb.h"

CryptStateOcb::CryptStateOcb() {
}

CryptStateOcb::~CryptStateOcb() {
}

#if defined(__LP64__)

#define BLOCKSIZE 2
#define SHIFTBITS 63
typedef quint64 subblock;

#define SWAPPED(x) SWAP64(x)
#else
#define BLOCKSIZE 4
#define SHIFTBITS 31
typedef quint32 subblock;
#if defined(Q_OS_WIN)
#define SWAPPED(x) _byteswap_ulong(x)
#else
#define SWAPPED(x) htonl(x)
#endif
#endif

typedef subblock keyblock[BLOCKSIZE];

#define HIGHBIT (1<<SHIFTBITS);

namespace {

void inline XOR(subblock *dst, const subblock *a, const subblock *b) {
	for (int i=0;i<BLOCKSIZE;i++) {
		dst[i] = a[i] ^ b[i];
	}
}

void inline S2(subblock *block) {
	subblock carry = SWAPPED(block[0]) >> SHIFTBITS;
	for (int i=0;i<BLOCKSIZE-1;i++)
		block[i] = SWAPPED((SWAPPED(block[i]) << 1) | (SWAPPED(block[i+1]) >> SHIFTBITS));
	block[BLOCKSIZE-1] = SWAPPED((SWAPPED(block[BLOCKSIZE-1]) << 1) ^(carry * 0x87));
}

void inline S3(subblock *block) {
	subblock carry = SWAPPED(block[0]) >> SHIFTBITS;
	for (int i=0;i<BLOCKSIZE-1;i++)
		block[i] ^= SWAPPED((SWAPPED(block[i]) << 1) | (SWAPPED(block[i+1]) >> SHIFTBITS));
	block[BLOCKSIZE-1] ^= SWAPPED((SWAPPED(block[BLOCKSIZE-1]) << 1) ^(carry * 0x87));
}

void inline ZERO(keyblock &block) {
	for (int i=0;i<BLOCKSIZE;i++)
		block[i]=0;
}

}

#define AESencrypt(src,dst,key) AES_encrypt(reinterpret_cast<const unsigned char *>(src),reinterpret_cast<unsigned char *>(dst), key);
#define AESdecrypt(src,dst,key) AES_decrypt(reinterpret_cast<const unsigned char *>(src),reinterpret_cast<unsigned char *>(dst), key);

void CryptStateOcb::doEncrypt(const unsigned char *plain, unsigned char *encrypted, unsigned int len, const unsigned char *nonce, unsigned char *tag) {
	keyblock checksum, delta, tmp, pad;

	// Initialize
	AESencrypt(nonce, delta, &encrypt_key);
	ZERO(checksum);

	while (len > AES_BLOCK_SIZE) {
		S2(delta);
		XOR(tmp, delta, reinterpret_cast<const subblock *>(plain));
		AESencrypt(tmp, tmp, &encrypt_key);
		XOR(reinterpret_cast<subblock *>(encrypted), delta, tmp);
		XOR(checksum, checksum, reinterpret_cast<const subblock *>(plain));
		len -= AES_BLOCK_SIZE;
		plain += AES_BLOCK_SIZE;
		encrypted += AES_BLOCK_SIZE;
	}

	S2(delta);
	ZERO(tmp);
	tmp[BLOCKSIZE - 1] = SWAPPED(len * 8);
	XOR(tmp, tmp, delta);
	AESencrypt(tmp, pad, &encrypt_key);
	memcpy(tmp, plain, len);
	memcpy(reinterpret_cast<unsigned char *>(tmp)+len, reinterpret_cast<const unsigned char *>(pad)+len, AES_BLOCK_SIZE - len);
	XOR(checksum, checksum, tmp);
	XOR(tmp, pad, tmp);
	memcpy(encrypted, tmp, len);

	S3(delta);
	XOR(tmp, delta, checksum);
	AESencrypt(tmp, tag, &encrypt_key);
}

void CryptStateOcb::doDecrypt(const unsigned char *encrypted, unsigned char *plain, unsigned int len, const unsigned char *nonce, unsigned char *tag) {
	keyblock checksum, delta, tmp, pad;

	// Initialize
	AESencrypt(nonce, delta, &encrypt_key);
	ZERO(checksum);

	while (len > AES_BLOCK_SIZE) {
		S2(delta);
		XOR(tmp, delta, reinterpret_cast<const subblock *>(encrypted));
		AESdecrypt(tmp, tmp, &decrypt_key);
		XOR(reinterpret_cast<subblock *>(plain), delta, tmp);
		XOR(checksum, checksum, reinterpret_cast<const subblock *>(plain));
		len -= AES_BLOCK_SIZE;
		plain += AES_BLOCK_SIZE;
		encrypted += AES_BLOCK_SIZE;
	}

	S2(delta);
	ZERO(tmp);
	tmp[BLOCKSIZE - 1] = SWAPPED(len * 8);
	XOR(tmp, tmp, delta);
	AESencrypt(tmp, pad, &encrypt_key);
	memset(tmp, 0, AES_BLOCK_SIZE);
	memcpy(tmp, encrypted, len);
	XOR(tmp, tmp, pad);
	XOR(checksum, checksum, tmp);
	memcpy(plain, tmp, len);

	S3(delta);
	XOR(tmp, delta, checksum);
	AESencrypt(tmp, tag, &encrypt_key);
}
