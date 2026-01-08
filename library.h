#ifndef BETACOM_LIBRARY_H
#define BETACOM_LIBRARY_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    BETA_COM_SUCCESS = 0,               // Operation successful
    BETA_COM_ERR_INVALID_ARGS = -1,     // NULL pointers passed as parameters
    BETA_COM_ERR_BUFFER_TOO_SMALL = -2, // Output or work buffer too small
    BETA_COM_ERR_DATA_CORRUPTED = -3,   // COBS format error (e.g., pointer out of bounds)
    BETA_COM_ERR_CRC_MISMATCH = -4,     // Calculated CRC does not match received CRC
    BETA_COM_ERR_MSG_TOO_SHORT = -5     // Decoded message too short to contain a CRC
} beta_com_err_t;

#define CRC16_REFLECTED_POLY 0xA001

/**
 * COBS (Consistent Overhead Byte Stuffing) encoding function.
 *
 * @param input Pointer to the input data buffer.
 * @param in_len Length of the input data buffer.
 * @param output Pointer to the output data buffer.
 * @param out_len Pointer to a variable that holds the size of the output buffer.
 *                On return, it will contain the length of the encoded data.
 * @return BETA_COM_SUCCESS (0) on success, error code (non-zero) on failure
 */
beta_com_err_t cobs_encode(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len);

/**
 * COBS (Consistent Overhead Byte Stuffing) decoding function.
 *
 * @param input Pointer to the input data buffer.
 * @param in_len Length of the input data buffer.
 * @param output Pointer to the output data buffer.
 * @param out_len Pointer to a variable that holds the size of the output buffer.
 *                On return, it will contain the length of the decoded data.
 * @return BETA_COM_SUCCESS (0) on success, error code (non-zero) on failure
 */
beta_com_err_t cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len);

/**
 * Calculates CRC16 checksum using the reflected polynomial 0xA001.
 *
 * @param data Pointer to the input data buffer.
 * @param length Length of the input data buffer.
 * @param crc_out Pointer to a variable where the calculated CRC16 will be stored.
 * @return BETA_COM_SUCCESS (0) on success, error code (non-zero) on failure
 */
beta_com_err_t calculate_crc16(const uint8_t *data, size_t length, uint16_t *crc_out);

/**
 * Checks CRC16 checksum using the reflected polynomial 0xA001.
 *
 * @param data Pointer to the input data buffer.
 * @param length Length of the input data buffer.
 * @param expected_crc The expected CRC16 checksum to compare against.
 * @return BETA_COM_SUCCESS (0) on success, error code (non-zero) on failure
 */
beta_com_err_t check_crc16(const uint8_t *data, size_t length, uint16_t expected_crc);

/**
 * Generates an encoded message by appending CRC16 checksum and encoding with COBS.
 *
 * @param input Pointer to the input data buffer.
 * @param in_len Length of the input data buffer.
 * @param output Pointer to the output data buffer.
 * @param out_len Pointer to a variable that holds the size of the output buffer. On return, it will contain the length of the encoded message.
 * @param work_buffer Pointer to a work buffer used during encoding.
 * @param work_len Length of the work buffer. Minimum required size is in_len + 2 (for CRC16).
 * @return BETA_COM_SUCCESS (0) on success, error code (non-zero) on failure
 */
beta_com_err_t generate_encoded_message(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len, uint8_t *work_buffer, size_t work_len);

/**
 * Decodes a message by decoding with COBS and verifying the CRC16 checksum.
 *
 * @param input Pointer to the input data buffer.
 * @param in_len Length of the input data buffer.
 * @param output Pointer to the output data buffer.
 * @param out_len Pointer to a variable that holds the size of the output buffer. On return, it will contain the length of the decoded data.
 * @return BETA_COM_SUCCESS (0) on success, error code (non-zero) on failure
 */
beta_com_err_t decode_message(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len);

#endif // BETACOM_LIBRARY_H