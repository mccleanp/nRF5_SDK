/**
 * Copyright (c) 2018 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @defgroup nrf_crypto_ecdsa_example
 * @{
 * @ingroup nrf_crypto_ecdsa
 * @brief ECDSA Example Application main file.
 *
 * This file contains the source code for a sample application that demonstrates using the
 * nrf_crypto library to do ECDSA signature generation and verification. Different backends can be 
 * used by adjusting @ref sdk_config.h accordingly.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sdk_common.h"
#include "nrf_assert.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_crypto.h"
#include "nrf_crypto_ecc.h"
#include "nrf_crypto_error.h"
#include "nrf_crypto_ecdsa.h"
#include "mem_manager.h"

/** @brief SHA-256 hash digest of the message "Hello bob!
 *
 * @note If you need to calculate a hash digest from message, please use
 *       @ref nrf_crypto_hash.
 */
static uint8_t m_hash[] =
{
    // SHA256("Hello Bob!")
    0x42, 0xba, 0x83, 0x54, 0xdb, 0x26, 0x3a, 0x6a,
    0x5a, 0x9f, 0x74, 0xd6, 0xb7, 0xce, 0xb4, 0xc9,
    0x62, 0xa3, 0xd8, 0xfd, 0x58, 0xa4, 0x19, 0x69,
    0xe5, 0x21, 0xeb, 0x02, 0x22, 0x45, 0x54, 0x15,
};


/** @brief Signature that Alice will generate and Bob will later verify.
 */
static nrf_crypto_ecdsa_secp256r1_signature_t m_signature;


/** @brief Size of the signature generated by Alice.
 */
static size_t m_signature_size;


//======================================== Print functions ========================================
//
// Utility functions used to print results generated in this examples.
//

static void print_array(uint8_t const * p_string, size_t size)
{
    #if NRF_LOG_ENABLED
    size_t i;
    NRF_LOG_RAW_INFO("    ");
    for(i = 0; i < size; i++)
    {
        NRF_LOG_RAW_INFO("%02x", p_string[i]);
    }
    #endif // NRF_LOG_ENABLED
}


static void print_hex(char const * p_msg, uint8_t const * p_data, size_t size)
{
    NRF_LOG_INFO(p_msg);
    print_array(p_data, size);
    NRF_LOG_RAW_INFO("\r\n");
}


#define DEMO_ERROR_CHECK(error)     \
do                                  \
{                                   \
    if (error != NRF_SUCCESS)       \
    {                               \
        NRF_LOG_ERROR("Error 0x%04X: %s", error, nrf_crypto_error_string_get(error));\
        APP_ERROR_CHECK(error);     \
    }                               \
} while(0)


//========================================= Alice's site =========================================
//
// This part of an example contains implementation of Alice's site. Alice have predefined private
// key which she needs to keep secret. She uses this private key to sign "Hello Bob!" message.
//


/** @brief Predefined example private key.
 *
 *  This private key contains some dummy data just to show the functionality. Is should never be
 *  placed in any practical usage. Is is not secure, because it is filled with ones (in HEX).
 */
static const uint8_t m_alice_raw_private_key[] =
{
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // This is an example. DO NOT USE THIS KEY!
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // This is an example. DO NOT USE THIS KEY!
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // This is an example. DO NOT USE THIS KEY!
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  // This is an example. DO NOT USE THIS KEY!
};


/** @brief Alice signs the message.
 */
static void alice_sign()
{
    static nrf_crypto_ecc_private_key_t alice_private_key;
    ret_code_t                          err_code = NRF_SUCCESS;

    NRF_LOG_INFO("Alice's signature generation");

    // Alice converts her raw private key to internal representation
    err_code = nrf_crypto_ecc_private_key_from_raw(&g_nrf_crypto_ecc_secp256r1_curve_info,
                                                   &alice_private_key,
                                                   m_alice_raw_private_key,
                                                   sizeof(m_alice_raw_private_key));
    DEMO_ERROR_CHECK(err_code);

    // Alice generates signature using ECDSA and SHA-256
    m_signature_size = sizeof(m_signature);
    err_code = nrf_crypto_ecdsa_sign(NULL,
                                     &alice_private_key,
                                     m_hash,
                                     sizeof(m_hash),
                                     m_signature,
                                     &m_signature_size);
    DEMO_ERROR_CHECK(err_code);

    // Alice can now send the message and its signature to Bob
    print_hex("Alice's message hash: ", m_hash, sizeof(m_hash));
    print_hex("Alice's signature: ", m_signature, m_signature_size);

    // Key deallocation
    err_code = nrf_crypto_ecc_private_key_free(&alice_private_key);
    DEMO_ERROR_CHECK(err_code);
}


//========================================== Bob's site ==========================================
//
// This part of the example contains implementation of Bobs's site. Bob has a public generated from
// Alice's private key. The public key is not secret and it could have been shared beforehand, by a
// key-exchange scheme, or by using another authenticated message exchange.
//
// He will use the public key it to verify authenticity of the message, i.e. check if the
// message is actually from Alice.
//


/** @brief Predefined example public key that is associated with example private key
 *  @ref m_alice_raw_private_key.
 */
static const uint8_t m_alice_raw_public_key[] =
{
    0x02, 0x17, 0xE6, 0x17, 0xF0, 0xB6, 0x44, 0x39,
    0x28, 0x27, 0x8F, 0x96, 0x99, 0x9E, 0x69, 0xA2,
    0x3A, 0x4F, 0x2C, 0x15, 0x2B, 0xDF, 0x6D, 0x6C,
    0xDF, 0x66, 0xE5, 0xB8, 0x02, 0x82, 0xD4, 0xED,
    0x19, 0x4A, 0x7D, 0xEB, 0xCB, 0x97, 0x71, 0x2D,
    0x2D, 0xDA, 0x3C, 0xA8, 0x5A, 0xA8, 0x76, 0x5A,
    0x56, 0xF4, 0x5F, 0xC7, 0x58, 0x59, 0x96, 0x52,
    0xF2, 0x89, 0x7C, 0x65, 0x30, 0x6E, 0x57, 0x94,
};


/** @brief Bob verifies the signature.
 */
void bob_verify()
{
    static nrf_crypto_ecc_public_key_t alice_public_key;
    ret_code_t                         err_code = NRF_SUCCESS;

    NRF_LOG_INFO("Bob's message verification");

    // Bob converts Alice's raw public key to internal representation
    err_code = nrf_crypto_ecc_public_key_from_raw(&g_nrf_crypto_ecc_secp256r1_curve_info,
                                                  &alice_public_key,
                                                  m_alice_raw_public_key,
                                                  sizeof(m_alice_raw_public_key));
    DEMO_ERROR_CHECK(err_code);

    // Bob verifies the message using ECDSA and SHA-256
    err_code = nrf_crypto_ecdsa_verify(NULL,
                                       &alice_public_key,
                                       m_hash,
                                       sizeof(m_hash),
                                       m_signature,
                                       m_signature_size);

    if (err_code == NRF_SUCCESS)
    {
        NRF_LOG_INFO("Signature is valid. Message is authentic.");
    }
    else if (err_code == NRF_ERROR_CRYPTO_ECDSA_INVALID_SIGNATURE)
    {
        NRF_LOG_WARNING("Signature is invalid. Message is not authentic.");
    }
    else
    {
        DEMO_ERROR_CHECK(err_code);
    }

    // Key deallocation
    err_code = nrf_crypto_ecc_public_key_free(&alice_public_key);
    DEMO_ERROR_CHECK(err_code);
}


//========================================= Example entry =========================================
//


/** @brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/** @brief Function for application main entry.
 */
int main(void)
{
    ret_code_t err_code = NRF_SUCCESS;

    log_init();

    NRF_LOG_INFO("ECDSA example started.\r\n");

    err_code = nrf_mem_init();
    DEMO_ERROR_CHECK(err_code);

    err_code = nrf_crypto_init();
    DEMO_ERROR_CHECK(err_code);

    alice_sign();  // Alice signs the message
    bob_verify();  // Bob verifies the signature

    NRF_LOG_INFO("ECDSA example executed successfully.");

    for (;;)
    {
    }
}


/** @}
 */
