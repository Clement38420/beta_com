#ifndef BETACOM_LIBRARY_H
#define BETACOM_LIBRARY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Encodes a byte buffer using the COBS algorithm.
 *
 * This function encodes the input data and automatically appends the
 * packet delimiter (0x00) at the end of the output buffer.
 *
 * @param input         Pointer to the raw data to encode.
 * @param in_len        Length of the raw data.
 * @param output        Pointer to the destination buffer.
 * @param max_out_len   Maximum capacity of the destination buffer.
 * * @return Total number of bytes written to output (including the trailing 0x00). Or an error code < 0.
 */
int32_t cobs_encode(const uint8_t *input, size_t in_len, uint8_t *output, size_t max_out_len);

/**
 * @brief Decodes a COBS-encoded frame.
 *
 * Reconstructs the original data from a COBS frame. Stops decoding if a
 * zero delimiter is encountered or the input length is reached.
 *
 * @param input         Pointer to the COBS-encoded data.
 * @param in_len        Length of the encoded data.
 * @param output        Pointer to the destination buffer.
 * @param max_out_len   Maximum capacity of the destination buffer.
 * * @return Length of the decoded payload.. Or an error code < 0.
 */
int32_t cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t max_out_len);

/**
 * @brief Calculates a CRC-16 checksum (Modbus/Reflected).
 *
 * Uses polynomial 0xA001 with an initial value of 0xFFFF.
 *
 * @param data      Pointer to the data buffer.
 * @param length    Length of the data buffer.
 * * @return uint16_t: The calculated CRC16 value. Returns 0 if data is NULL.
 */
uint16_t calculate_crc16(const uint8_t *data, size_t length);

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

#ifdef __cplusplus
}
#endif

#endif // BETACOM_LIBRARY_H