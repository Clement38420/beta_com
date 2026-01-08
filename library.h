#ifndef BETACOM_LIBRARY_H
#define BETACOM_LIBRARY_H

#include <stdint.h>
#include <stddef.h>

#define CRC16_REFLECTED_POLY 0xA001

/**
 * COBS (Consistent Overhead Byte Stuffing) encoding function.
 *
 * @param input Pointer to the input data buffer.
 * @param in_len Length of the input data buffer.
 * @param output Pointer to the output data buffer.
 * @param out_len Pointer to a variable that holds the size of the output buffer.
 *                On return, it will contain the length of the encoded data.
 * @return 0 on success, non-zero on failure (e.g., insufficient output buffer size).
 */
int cobs_encode(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len);

/**
 * COBS (Consistent Overhead Byte Stuffing) decoding function.
 *
 * @param input Pointer to the input data buffer.
 * @param in_len Length of the input data buffer.
 * @param output Pointer to the output data buffer.
 * @param out_len Pointer to a variable that holds the size of the output buffer.
 *                On return, it will contain the length of the decoded data.
 * @return 0 on success, non-zero on failure (e.g., insufficient output buffer size).
 */
int cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len);

/**
 * Calculate CRC16 checksum using the reflected polynomial 0xA001.
 *
 * @param data Pointer to the input data buffer.
 * @param length Length of the input data buffer.
 * @param crc_out Pointer to a variable where the calculated CRC16 will be stored.
 * @return 0 on success, non-zero on failure (e.g., null pointers).
 */
int calculate_crc16(const uint8_t *data, size_t length, uint16_t *crc_out);

/**
 * Check CRC16 checksum using the reflected polynomial 0xA001.
 *
 * @param data Pointer to the input data buffer.
 * @param length Length of the input data buffer.
 * @param expected_crc The expected CRC16 checksum to compare against.
 * @return 0 if the checksum matches, non-zero if it does not match or on failure.
 */
int check_crc16(const uint8_t *data, size_t length, uint16_t expected_crc);

#endif // BETACOM_LIBRARY_H