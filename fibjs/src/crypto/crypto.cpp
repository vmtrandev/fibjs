/*
 * crypto.cpp
 *
 *  Created on: Apr 9, 2014
 *      Author: lion
 */

#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#include "object.h"
#include "ifs/crypto.h"
#include "Cipher.h"
#include "Buffer.h"
#include "Digest.h"
#include "PKey.h"
#include "ECKey.h"
#include "PKey_rsa.h"
#include "X509Cert.h"
#include "X509Crl.h"
#include "X509Req.h"
#include "ssl.h"
#include <time.h>
#include <string.h>
#include <mbedtls/mbedtls/pkcs5.h>
#include "md_api.h"

namespace fibjs {

DECLARE_MODULE(crypto);

result_t crypto_base::createHash(exlib::string algo, obj_ptr<Digest_base>& retVal)
{
    algo.toupper();
    if (algo == "RMD160")
        algo = "RIPEMD160";

    mbedtls_md_type_t algo_id = _md_type_from_string(algo.c_str());
    if (algo_id == MBEDTLS_MD_NONE)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    retVal = new Digest(algo_id);

    return 0;
}

result_t crypto_base::createHmac(exlib::string algo, Buffer_base* key,
    obj_ptr<Digest_base>& retVal)
{
    algo.toupper();
    if (algo == "RMD160")
        algo = "RIPEMD160";

    mbedtls_md_type_t algo_id = _md_type_from_string(algo.c_str());
    if (algo_id == MBEDTLS_MD_NONE)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    return hash_base::hmac(algo_id, key, NULL, retVal);
}

result_t crypto_base::loadCert(exlib::string filename, obj_ptr<X509Cert_base>& retVal)
{
    return X509Cert::loadFile(filename, retVal);
}

result_t crypto_base::loadCrl(exlib::string filename, obj_ptr<X509Crl_base>& retVal)
{
    return X509Crl::loadFile(filename, retVal);
}

result_t crypto_base::loadReq(exlib::string filename, obj_ptr<X509Req_base>& retVal)
{
    return X509Req::loadFile(filename, retVal);
}

result_t crypto_base::loadPKey(exlib::string filename, obj_ptr<PKey_base>& retVal)
{
    return PKey::loadFile(filename, retVal);
}

result_t crypto_base::randomBytes(int32_t size, obj_ptr<Buffer_base>& retVal,
    AsyncEvent* ac)
{
    return pseudoRandomBytes(size, retVal, ac);
}

result_t crypto_base::simpleRandomBytes(int32_t size, obj_ptr<Buffer_base>& retVal,
    AsyncEvent* ac)
{
    if (size < 1)
        return CHECK_ERROR(CALL_E_OUTRANGE);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    exlib::string strBuf;

    strBuf.resize(size);
    char* ptr = strBuf.c_buffer();
    int32_t i;

    for (i = 0; i < size; i++)
        ptr[i] = rand() % 256;

    retVal = new Buffer(strBuf);

    return 0;
}

result_t crypto_base::pseudoRandomBytes(int32_t size, obj_ptr<Buffer_base>& retVal,
    AsyncEvent* ac)
{
    if (size < 1)
        return CHECK_ERROR(CALL_E_OUTRANGE);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    int32_t i, ret;
    mbedtls_entropy_context entropy;
    unsigned char buf[MBEDTLS_ENTROPY_BLOCK_SIZE];
    exlib::string strBuf;

    strBuf.resize(size);
    char* _strBuf = strBuf.c_buffer();

    mbedtls_entropy_init(&entropy);

    for (i = 0; i < size; i += sizeof(buf)) {
        ret = mbedtls_entropy_func(&entropy, buf, sizeof(buf));
        if (ret != 0) {
            mbedtls_entropy_free(&entropy);
            return CHECK_ERROR(_ssl::setError(ret));
        }

        memcpy(&_strBuf[i], buf, size - i > (int32_t)sizeof(buf) ? (int32_t)sizeof(buf) : size - i);
    }

    mbedtls_entropy_free(&entropy);
    retVal = new Buffer(strBuf);

    return 0;
}

result_t crypto_base::randomFill(Buffer_base* buffer, int32_t offset, int32_t size,
    obj_ptr<Buffer_base>& retVal, AsyncEvent* ac)
{
    int32_t len;
    buffer->get_length(len);

    if (offset < 0 || offset > len)
        return CHECK_ERROR(CALL_E_OUTRANGE);

    if (size < 0)
        size = len - offset;
    else if (size + offset > len)
        return CHECK_ERROR(CALL_E_OUTRANGE);

    if (size == 0) {
        retVal = buffer;
        return 0;
    }

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    obj_ptr<Buffer_base> rand;
    simpleRandomBytes(size, rand, ac);

    return buffer->fill(rand, offset, offset + size, retVal);
}

inline int32_t _max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

inline int32_t _min(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

char* randomart(const unsigned char* dgst_raw, size_t dgst_raw_len,
    exlib::string title, int32_t size)
{
    const char augmentation_string[] = " .o+=*BOX@%&#/^SE";
    char *retval, *p;
    unsigned char* field;
    int32_t i, b, n;
    int32_t x, y;
    const size_t len = sizeof(augmentation_string) - 2;
    int32_t fieldY = size + 1;
    int32_t fieldX = size * 2 + 1;

    retval = (char*)calloc(1, (fieldX + 3) * (fieldY + 2));
    if (retval == NULL)
        return NULL;

    field = (unsigned char*)calloc(1, fieldX * fieldY);
    x = fieldX / 2;
    y = fieldY / 2;

    for (i = 0; i < (int32_t)dgst_raw_len; i++) {
        int32_t input;

        input = dgst_raw[i];
        for (b = 0; b < 4; b++) {
            x += (input & 0x1) ? 1 : -1;
            y += (input & 0x2) ? 1 : -1;

            x = _max(x, 0);
            y = _max(y, 0);
            x = _min(x, fieldX - 1);
            y = _min(y, fieldY - 1);

            if (field[x * fieldY + y] < len - 2)
                field[x * fieldY + y]++;
            input = input >> 2;
        }
    }

    field[(fieldX / 2) * fieldY + fieldY / 2] = len - 1;
    field[x * fieldY + y] = len;

    p = retval;
    *p++ = '+';
    for (i = 0; i < fieldX; i++)
        *p++ = '-';
    *p++ = '+';
    *p++ = '\n';

    n = (int32_t)title.length();
    if (n > 0) {
        if (n > fieldX - 2)
            n = fieldX - 2;
        p = retval + (fieldX - n) / 2;

        *p++ = '[';
        memcpy(p, title.c_str(), n);
        p += n;
        *p++ = ']';
        p = retval + fieldX + 3;
    }

    for (y = 0; y < fieldY; y++) {
        *p++ = '|';
        for (x = 0; x < fieldX; x++)
            *p++ = augmentation_string[_min(field[x * fieldY + y], len)];
        *p++ = '|';
        *p++ = '\n';
    }

    *p++ = '+';
    for (i = 0; i < fieldX; i++)
        *p++ = '-';
    *p++ = '+';

    free(field);

    return retval;
}

result_t crypto_base::randomArt(Buffer_base* data, exlib::string title,
    int32_t size, exlib::string& retVal)
{
    if (size < 1 || size > 128)
        return CHECK_ERROR(CALL_E_OUTRANGE);

    exlib::string buf;

    data->toString(buf);
    char* str = randomart((const unsigned char*)buf.c_str(), buf.length(),
        title, size);

    if (str) {
        retVal = str;
        free(str);
    }

    return 0;
}

result_t crypto_base::generateKey(int32_t size, obj_ptr<PKey_base>& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    return PKey_rsa::generateKey(size, retVal);
}

result_t crypto_base::generateKey(exlib::string curve, obj_ptr<PKey_base>& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    return ECKey::generateKey(curve, retVal);
}

inline int pkcs5_pbkdf1(mbedtls_md_context_t* ctx, const unsigned char* password,
    size_t plen, const unsigned char* salt, size_t slen,
    unsigned int iteration_count,
    uint32_t key_length, unsigned char* output)
{
    int ret;
    unsigned int i;
    unsigned char md1[MBEDTLS_MD_MAX_SIZE];
    unsigned char work[MBEDTLS_MD_MAX_SIZE];
    unsigned char md_size = mbedtls_md_get_size(ctx->md_info);
    size_t use_len;
    unsigned char* out_p = output;
    bool bFirst = true;

    if (iteration_count > 0xFFFFFFFF)
        return (MBEDTLS_ERR_PKCS5_BAD_INPUT_DATA);

    while (key_length) {
        if ((ret = _md_starts(ctx)) != 0)
            return (ret);

        if (!bFirst)
            if ((ret = _md_update(ctx, md1, md_size)) != 0)
                return (ret);
        bFirst = false;

        if ((ret = _md_update(ctx, password, plen)) != 0)
            return (ret);

        if ((ret = _md_update(ctx, salt, slen)) != 0)
            return (ret);

        if ((ret = _md_finish(ctx, work)) != 0)
            return (ret);

        memcpy(md1, work, md_size);

        for (i = 1; i < iteration_count; i++) {
            if ((ret = _md_starts(ctx)) != 0)
                return (ret);

            if ((ret = _md_update(ctx, md1, md_size)) != 0)
                return (ret);

            if ((ret = _md_finish(ctx, work)) != 0)
                return (ret);

            memcpy(md1, work, md_size);
        }

        use_len = (key_length < md_size) ? key_length : md_size;
        memcpy(out_p, work, use_len);

        key_length -= (uint32_t)use_len;
        out_p += use_len;
    }

    return (0);
}

result_t crypto_base::pbkdf1(Buffer_base* password, Buffer_base* salt, int32_t iterations,
    int32_t size, int32_t algo, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac)
{
    if (iterations < 1 || size < 1)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    if (algo < hash_base::C_MD5 || algo > hash_base::C_SM3)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    exlib::string str_pass;
    exlib::string str_salt;
    exlib::string str_key;

    password->toString(str_pass);
    salt->toString(str_salt);
    str_key.resize(size);

    mbedtls_md_context_t ctx;
    _md_setup(&ctx, (mbedtls_md_type_t)algo, 1);
    pkcs5_pbkdf1(&ctx, (const unsigned char*)str_pass.c_str(), str_pass.length(),
        (const unsigned char*)str_salt.c_str(), str_salt.length(),
        iterations, size, (unsigned char*)str_key.c_buffer());
    mbedtls_md_free(&ctx);

    retVal = new Buffer(str_key);

    return 0;
}

result_t crypto_base::pbkdf1(Buffer_base* password, Buffer_base* salt, int32_t iterations,
    int32_t size, exlib::string algoName, obj_ptr<Buffer_base>& retVal,
    AsyncEvent* ac)
{
    if (iterations < 1 || size < 1)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    algoName.toupper();
    mbedtls_md_type_t algo_id = _md_type_from_string(algoName.c_str());
    if (algo_id == MBEDTLS_MD_NONE)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    return pbkdf1(password, salt, iterations, size, algo_id, retVal, ac);
}

result_t crypto_base::pbkdf2(Buffer_base* password, Buffer_base* salt, int32_t iterations,
    int32_t size, int32_t algo, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac)
{
    if (iterations < 1 || size < 1)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    if (algo < hash_base::C_MD5 || algo > hash_base::C_SM3)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    exlib::string str_pass;
    exlib::string str_salt;
    exlib::string str_key;

    password->toString(str_pass);
    salt->toString(str_salt);
    str_key.resize(size);

    mbedtls_md_context_t ctx;
    _md_setup(&ctx, (mbedtls_md_type_t)algo, 1);
    mbedtls_pkcs5_pbkdf2_hmac(&ctx, (const unsigned char*)str_pass.c_str(), str_pass.length(),
        (const unsigned char*)str_salt.c_str(), str_salt.length(),
        iterations, size, (unsigned char*)str_key.c_buffer());
    mbedtls_md_free(&ctx);

    retVal = new Buffer(str_key);

    return 0;
}

result_t crypto_base::pbkdf2(Buffer_base* password, Buffer_base* salt, int32_t iterations,
    int32_t size, exlib::string algoName, obj_ptr<Buffer_base>& retVal,
    AsyncEvent* ac)
{
    if (iterations < 1 || size < 1)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    algoName.toupper();
    mbedtls_md_type_t algo_id = _md_type_from_string(algoName.c_str());
    if (algo_id == MBEDTLS_MD_NONE)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    return pbkdf2(password, salt, iterations, size, algo_id, retVal, ac);
}

extern obj_ptr<NArray> g_hashes;

result_t crypto_base::getHashes(v8::Local<v8::Array>& retVal)
{
    v8::Local<v8::Value> v;
    g_hashes->valueOf(v);

    retVal = v8::Local<v8::Array>::Cast(v);

    return 0;
}
}
