/* Copyright (C) 2011, Benjamin Jemlich <pcgod@users.sourceforge.net>

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

#include "CryptStateEax.h"

namespace {

#if defined(__LP64__)
# define BLOCKSIZE 2
# define SHIFTBITS 63
typedef uint64_t subblock;
# define SWAPPED(x) SWAP64(x)
#else
# define BLOCKSIZE 4
# define SHIFTBITS 31
typedef uint32_t subblock;
# if defined(Q_OS_WIN)
#  define SWAPPED(x) _byteswap_ulong(x)
# else
#  define SWAPPED(x) htonl(x)
# endif
#endif

typedef subblock keyblock[BLOCKSIZE];

#define BLOCKBITS ((AES_BLOCK_SIZE / BLOCKSIZE) * 8)
#define HIGHBIT (1L<<SHIFTBITS);

void inline XOR(subblock* dst, const subblock* a, const subblock* b) {
	for (int i = 0; i < BLOCKSIZE; i++)
		dst[i] = a[i] ^ b[i];
}

void inline ZERO(subblock* block) {
	for (int i = 0; i < BLOCKSIZE; i++)
		block[i] = 0;
}

void inline SWAP(subblock* block) {
	for (int i = 0; i < BLOCKSIZE; i++)
		block[i] = SWAPPED(block[i]);
}

void inline SHL(subblock* block) {
	SWAP(block);

	int carry = block[0] >> SHIFTBITS;
	for (int i = 0; i < BLOCKSIZE - 1; ++i)
		block[i] = (block[i] << 1) | (block[i + 1] >> SHIFTBITS);

	block[BLOCKSIZE - 1] = (block[BLOCKSIZE - 1] << 1) ^ (carry * 0x87);

	SWAP(block);
}

#define AESencrypt(src,dst,key) AES_encrypt(reinterpret_cast<const unsigned char *>(src),reinterpret_cast<unsigned char *>(dst), key);
#define AESdecrypt(src,dst,key) AES_decrypt(reinterpret_cast<const unsigned char *>(src),reinterpret_cast<unsigned char *>(dst), key);

}  // namespace

CryptStateEax::CryptStateEax() {
}

CryptStateEax::~CryptStateEax() {
}

void CryptStateEax::genKey() {
	CryptStateBase::genKey();
	initializeL();
}


void CryptStateEax::setKey(const unsigned char* rkey, const unsigned char* eiv, const unsigned char* div) {
	CryptStateBase::setKey(rkey, eiv, div);
	initializeL();
}

void CryptStateEax::initializeL() {
	// L
	ZERO(reinterpret_cast<subblock*>(lu));
	AESencrypt(lu, lu, &encrypt_key);
	// Lu
	SHL(reinterpret_cast<subblock*>(lu));

	memcpy(lu2, lu, AES_BLOCK_SIZE);

	// Lu2
	SHL(reinterpret_cast<subblock*>(lu2));
}

void CryptStateEax::doEncrypt(const unsigned char *plain, unsigned char *encrypted, unsigned int len, const unsigned char *nonce, unsigned char *tag) {
	eax_encrypt(plain, encrypted, len, nonce, AES_BLOCK_SIZE, NULL, 0, tag, 3);
}

void CryptStateEax::doDecrypt(const unsigned char *encrypted, unsigned char *plain, unsigned int len, const unsigned char *nonce, unsigned char *tag) {
	eax_decrypt(encrypted, plain, len, nonce, AES_BLOCK_SIZE, NULL, 0, tag, 3);
}

void CryptStateEax::omac(const unsigned char* data, unsigned int len, unsigned char* mac) {
	keyblock y, tmp, x;

	ZERO(y);

	while (len > AES_BLOCK_SIZE) {
		XOR(tmp, reinterpret_cast<const subblock*>(data), y);
		AESencrypt(tmp, y, &encrypt_key);
		len -= AES_BLOCK_SIZE;
		data += AES_BLOCK_SIZE;
	}

	ZERO(tmp);
	memcpy(tmp, data, len);

	subblock *l = reinterpret_cast<subblock*>(lu);

	if (len < AES_BLOCK_SIZE) {
		int bit = (len * 8) + 1;
		tmp[bit / BLOCKBITS] |= SWAPPED(1L << (BLOCKBITS - (bit % BLOCKBITS)));

		l = reinterpret_cast<subblock*>(lu2);
	}

	XOR(x, tmp, y);
	XOR(x, x, l);
	AESencrypt(x, mac, &encrypt_key);
}

void CryptStateEax::ctr(const unsigned char* in, unsigned char* out, unsigned int len, AES_KEY* key, unsigned char* nonce) {
	unsigned char counter[AES_BLOCK_SIZE];
	keyblock tmp;

	memcpy(counter, nonce, AES_BLOCK_SIZE);

	while (len > 0) {
		AESencrypt(counter, tmp, key);

		int tmplen = len > AES_BLOCK_SIZE ? AES_BLOCK_SIZE : len;
		XOR(reinterpret_cast<subblock*>(out), reinterpret_cast<const subblock*>(in), tmp);

		len -= tmplen;
		in += tmplen;
		out += tmplen;

		for (int i = AES_BLOCK_SIZE - 1; i >= 0; --i)
			if (++counter[i])
				break;
	}
}

void CryptStateEax::eax_encrypt(const unsigned char* plain, unsigned char* encrypted, unsigned int msglen, const unsigned char* nonce, unsigned int noncelen, const unsigned char* header, unsigned int headerlen, unsigned char* tag, unsigned int taglen) {
	keyblock noncemac, headermac, datamac, tmptag;

	// FIXME(pcgod): This will fail if msglen < headerlen < noncelen
	unsigned char* buffer = static_cast<unsigned char*>(malloc(16 + msglen));
	memset(buffer, 0, 15);

	buffer[15] = 0;
	memcpy(buffer + 16, nonce, noncelen);
	omac(buffer, 16 + noncelen, reinterpret_cast<unsigned char*>(noncemac));

	buffer[15] = 1;
	memcpy(buffer + 16, header, headerlen);
	omac(buffer, 16 + headerlen, reinterpret_cast<unsigned char*>(headermac));

	ctr(plain, encrypted, msglen, &encrypt_key, reinterpret_cast<unsigned char*>(noncemac));

	// FIXME(pcgod): copying the data is unneccesary
	buffer[15] = 2;
	memcpy(buffer + 16, encrypted, msglen);
	omac(buffer, 16 + msglen, reinterpret_cast<unsigned char*>(datamac));

	for (unsigned int i = 0; i < BLOCKSIZE; ++i) {
		tmptag[i] = noncemac[i] ^ datamac[i] ^ headermac[i];
	}

	memcpy(tag, tmptag, taglen);
	free(buffer);
}

bool CryptStateEax::eax_decrypt(const unsigned char* encrypted, unsigned char* plain, unsigned int msglen, const unsigned char* nonce, unsigned int noncelen, const unsigned char* header, unsigned int headerlen, unsigned char* tag, unsigned int taglen) {
	keyblock noncemac, headermac, datamac, tmptag;

	bool ret = false;
	// FIXME(pcgod): This will fail if msglen < headerlen < noncelen
	unsigned char* buffer = static_cast<unsigned char*>(malloc(16 + msglen));
	memset(buffer, 0, 15);

	buffer[15] = 0;
	memcpy(buffer + 16, nonce, noncelen);
	omac(buffer, 16 + noncelen, reinterpret_cast<unsigned char*>(noncemac));

	buffer[15] = 1;
	memcpy(buffer + 16, header, headerlen);
	omac(buffer, 16 + headerlen, reinterpret_cast<unsigned char*>(headermac));

	// FIXME(pcgod): copying the data is unneccesary
	buffer[15] = 2;
	memcpy(buffer + 16, encrypted, msglen);
	omac(buffer, 16 + msglen, reinterpret_cast<unsigned char*>(datamac));

	for (unsigned int i = 0; i < BLOCKSIZE; ++i) {
		tmptag[i] = noncemac[i] ^ datamac[i] ^ headermac[i];
	}

	if (memcmp(tmptag, tag, taglen) != 0) {
		ret = false;
		goto exit;
	}

	memcpy(tag, tmptag, taglen);

	ctr(encrypted, plain, msglen, &encrypt_key, reinterpret_cast<unsigned char*>(noncemac));
	ret = true;

exit:
	free(buffer);
	return ret;
}

