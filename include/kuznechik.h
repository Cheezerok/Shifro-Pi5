/**
 * @file
 */

#ifndef KUZNECHIK_H
#define KUZNECHIK_H

#include "type.h"

/* Magma cryptographic algorithm key size definition */
#define KUZNECHIK_KEY_SIZE          256
#define KUZNECHIK_KEY_BYTES         (KUZNECHIK_KEY_SIZE / 8)
#define KUZNECHIK_KEY_WORDS         (KUZNECHIK_KEY_SIZE / 32)

/* Magma cryptographic algorithm block size definition */
#define KUZNECHIK_BLOCK_SIZE        128
#define KUZNECHIK_BLOCK_BYTES       (KUZNECHIK_BLOCK_SIZE / 8)
#define KUZNECHIK_BLOCK_WORDS       (KUZNECHIK_BLOCK_SIZE / 32)

typedef u8 kuznechik_expanded_key_t[10][16];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ��������� �����
 * @param[in] masterKey ������ ����
 * @param[out] keys ������ ����������� ������
 * @return 0 ���� ��� �������������� ������ �������
 * @return -1 ���� ��������� ������
 */
int kuznechik_expkey(unsigned char* masterKey, unsigned char* keys);

/**
 * @brief ���������� ������������ �����
 * @param[in] plainText �������� ����
 * @param[out] chipherText ������������� ����
 * @param[in] keys ����������� �����
 * @return 0 ���� ��� �������������� ������ �������
 * @return -1 ���� ��������� ������
 */
int kuznechik_encrypt(unsigned char* plainText, unsigned char* chipherText, unsigned char* keys);

/**
 * @brief ��������� ������������� �����
 * @param[in] chipherText ������������� ����
 * @param[out] plainText �������������� ����
 * @param[in] keys ����������� �����
 * @return 0 ���� ��� �������������� ������ �������
 * @return -1 ���� ��������� ������
 */
int kuznechik_decrypt(unsigned char* chipherText, unsigned char* plainText, unsigned char* keys);

#ifdef __cplusplus
}
#endif

#endif
