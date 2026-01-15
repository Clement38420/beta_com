#include "beta_com.h"

#include <string.h>

int32_t cobs_encode(const uint8_t *input, size_t in_len, uint8_t *output, size_t max_out_len) {
    if (input == NULL || output == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    const uint8_t *p_in = input;
    const uint8_t *p_in_end = input + in_len;
    uint8_t *p_out = output;
    uint8_t *p_out_end = output + max_out_len;

    if (max_out_len < 2) return BETA_COM_ERR_BUFFER_TOO_SMALL; // Need at least space for code byte and trailing zero

    uint8_t *p_code = p_out++; // Reserve space for the first code byte
    uint8_t code = 1;

    while (p_in < p_in_end) {
        if (*p_in == 0) {
            *p_code = code;
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            p_code = p_out++;
            code = 1;
        } else {
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            *p_out++ = *p_in;
            code++;

            if (code == 0xFF) { // Maximum code value reached
                *p_code = code;
                if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                p_code = p_out++;
                code = 1;
            }
        }
        p_in++;
    }

    // Frame ending
    *p_code = code;

    if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
    *p_out++ = 0x00; // Add the trailing zero byte

    return (int32_t)(p_out - output);
}

int32_t cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t max_out_len) {
    if (input == NULL || output == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    const uint8_t *p_in = input;
    const uint8_t *p_in_end = input + in_len;
    uint8_t *p_out = output;
    uint8_t *p_out_end = output + max_out_len;


    while (p_in < p_in_end) {
        uint8_t code = *p_in++;

        if (code == 0) break;

        if (p_in + code - 1 > p_in_end) return BETA_COM_ERR_DATA_CORRUPTED;

        for (uint8_t i = 1; i < code; i++) {
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            *p_out++ = *p_in++;
        }

        if (code < 0xFF && p_in < p_in_end && *p_in != 0x00) {
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            *p_out++ = 0x00;
        }
    }

    return (int32_t)(p_out - output);
}

uint16_t calculate_crc16(const uint8_t *data, size_t length) {
    if (data == NULL) {
        return 0;
    }

    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i<length; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC16_REFLECTED_POLY;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

    uint16_t crc;
    beta_com_err_t crc_err_code = calculate_crc16(input, in_len, &crc);
    if (crc_err_code != 0) {
        return crc_err_code;
    }

    if (in_len + 2 > work_len) {
      return BETA_COM_ERR_BUFFER_TOO_SMALL; // Not enough space for input data + CRC
    }

    memcpy(work_buffer, input, in_len);

    work_buffer[in_len] = (uint8_t)(crc & 0xFF); // Low byte
    work_buffer[in_len + 1] = (uint8_t)((crc >> 8) & 0xFF); // High byte

    const int result = cobs_encode(work_buffer,  in_len + 2, output, out_len);

    if (result != 0) {
        return result;
    }

    return BETA_COM_SUCCESS;
}

beta_com_err_t decode_message(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len) {
    beta_com_err_t cobs_err_code = cobs_decode(input,  in_len, output, out_len);

    if (cobs_err_code != 0) {
        return cobs_err_code;
    }

    if (*out_len < 2) {
        return BETA_COM_ERR_MSG_TOO_SHORT;
    }

    beta_com_err_t crc_err_code = check_crc16(output, *out_len - 2, ((uint16_t)output[(*out_len) - 1] << 8) | output[(*out_len) - 2]);
    if (crc_err_code != 0) {
        return crc_err_code;
    }

    *out_len -= 2;

    return BETA_COM_SUCCESS;
}