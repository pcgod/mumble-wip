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

#include "CryptStateBase.h"
#include "Net.h"

CryptStateBase::CryptStateBase() {
	for (int i=0;i<0x100;i++)
		decrypt_history[i] = 0;
	bInit = false;
	uiGood=uiLate=uiLost=uiResync=0;
	uiRemoteGood=uiRemoteLate=uiRemoteLost=uiRemoteResync=0;
}

CryptStateBase::~CryptStateBase() {
}

bool CryptStateBase::isValid() const {
	return bInit;
}

void CryptStateBase::genKey() {
	RAND_bytes(raw_key, AES_BLOCK_SIZE);
	RAND_bytes(encrypt_iv, AES_BLOCK_SIZE);
	RAND_bytes(decrypt_iv, AES_BLOCK_SIZE);
	AES_set_encrypt_key(raw_key, 128, &encrypt_key);
	AES_set_decrypt_key(raw_key, 128, &decrypt_key);
	bInit = true;
}

void CryptStateBase::setKey(const unsigned char *rkey, const unsigned char *eiv, const unsigned char *div) {
	memcpy(raw_key, rkey, AES_BLOCK_SIZE);
	memcpy(encrypt_iv, eiv, AES_BLOCK_SIZE);
	memcpy(decrypt_iv, div, AES_BLOCK_SIZE);
	AES_set_encrypt_key(raw_key, 128, &encrypt_key);
	AES_set_decrypt_key(raw_key, 128, &decrypt_key);
	bInit = true;
}

void CryptStateBase::setDecryptIV(const unsigned char *iv) {
	memcpy(decrypt_iv, iv, AES_BLOCK_SIZE);
}

const unsigned char *CryptStateBase::getKey() const {
	return raw_key;
}

const unsigned char *CryptStateBase::getDecryptIV() const {
	return decrypt_iv;
}

const unsigned char *CryptStateBase::getEncryptIV() const {
	return encrypt_iv;
}

void CryptStateBase::encrypt(const unsigned char *source, unsigned char *dst, unsigned int plain_length) {
	unsigned char tag[AES_BLOCK_SIZE];

	// First, increase our IV.
	for (int i=0;i<AES_BLOCK_SIZE;i++)
		if (++encrypt_iv[i])
			break;

	doEncrypt(source, dst+4, plain_length, encrypt_iv, tag);

	dst[0] = encrypt_iv[0];
	dst[1] = tag[0];
	dst[2] = tag[1];
	dst[3] = tag[2];
}

bool CryptStateBase::decrypt(const unsigned char *source, unsigned char *dst, unsigned int crypted_length) {
	if (crypted_length < 4)
		return false;

	unsigned int plain_length = crypted_length - 4;

	unsigned char saveiv[AES_BLOCK_SIZE];
	unsigned char ivbyte = source[0];
	bool restore = false;
	unsigned char tag[AES_BLOCK_SIZE];

	int lost = 0;
	int late = 0;

	memcpy(saveiv, decrypt_iv, AES_BLOCK_SIZE);

	if (((decrypt_iv[0] + 1) & 0xFF) == ivbyte) {
		// In order as expected.
		if (ivbyte > decrypt_iv[0]) {
			decrypt_iv[0] = ivbyte;
		} else if (ivbyte < decrypt_iv[0]) {
			decrypt_iv[0] = ivbyte;
			for (int i=1;i<AES_BLOCK_SIZE;i++)
				if (++decrypt_iv[i])
					break;
		} else {
			return false;
		}
	} else {
		// This is either out of order or a repeat.

		int diff = ivbyte - decrypt_iv[0];
		if (diff > 128)
			diff = diff-256;
		else if (diff < -128)
			diff = diff+256;

		if ((ivbyte < decrypt_iv[0]) && (diff > -30) && (diff < 0)) {
			// Late packet, but no wraparound.
			late = 1;
			lost = -1;
			decrypt_iv[0] = ivbyte;
			restore = true;
		} else if ((ivbyte > decrypt_iv[0]) && (diff > -30) && (diff < 0)) {
			// Last was 0x02, here comes 0xff from last round
			late = 1;
			lost = -1;
			decrypt_iv[0] = ivbyte;
			for (int i=1;i<AES_BLOCK_SIZE;i++)
				if (decrypt_iv[i]--)
					break;
			restore = true;
		} else if ((ivbyte > decrypt_iv[0]) && (diff > 0)) {
			// Lost a few packets, but beyond that we're good.
			lost = ivbyte - decrypt_iv[0] - 1;
			decrypt_iv[0] = ivbyte;
		} else if ((ivbyte < decrypt_iv[0]) && (diff > 0)) {
			// Lost a few packets, and wrapped around
			lost = 256 - decrypt_iv[0] + ivbyte - 1;
			decrypt_iv[0] = ivbyte;
			for (int i=1;i<AES_BLOCK_SIZE;i++)
				if (++decrypt_iv[i])
					break;
		} else {
			return false;
		}

		if (decrypt_history[decrypt_iv[0]] == decrypt_iv[1]) {
			memcpy(decrypt_iv, saveiv, AES_BLOCK_SIZE);
			return false;
		}
	}

	memcpy(tag, source + 1, 3);
	doDecrypt(source+4, dst, plain_length, decrypt_iv, tag);

	if (memcmp(tag, source+1, 3) != 0) {
		memcpy(decrypt_iv, saveiv, AES_BLOCK_SIZE);
		return false;
	}
	decrypt_history[decrypt_iv[0]] = decrypt_iv[1];

	if (restore)
		memcpy(decrypt_iv, saveiv, AES_BLOCK_SIZE);

	uiGood++;
	uiLate += late;
	uiLost += lost;

	tLastGood.restart();
	return true;
}

