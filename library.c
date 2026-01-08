#include "library.h"

#include <string.h>

int cobs_encode(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len) {
    if (input == NULL || output == NULL || out_len == NULL) {
        return 1;
    }

    const uint8_t *p_in = input;
    const uint8_t *p_in_end = input + in_len;
    uint8_t *p_out = output;
    uint8_t *p_out_end = output + *out_len;

    if (*out_len < 2) return 1;

    uint8_t *p_code = p_out++; // Reserve space for the first code byte
    uint8_t code = 1;

    while (p_in < p_in_end) {
        if (*p_in == 0) {
            *p_code = code;
            if (p_out >= p_out_end) return 1;
            p_code = p_out++;
            code = 1;
        } else {
            if (p_out >= p_out_end) return 1;
            *p_out++ = *p_in;
            code++;

            if (code == 0xFF) { // Maximum code value reached
                *p_code = code;
                if (p_out >= p_out_end) return 1;
                p_code = p_out++;
                code = 1;
            }
        }
        p_in++;
    }

    // Frame ending
    *p_code = code;

    if (p_out >= p_out_end) return 1;
    *p_out++ = 0x00; // Add the trailing zero byte

    *out_len = (size_t)(p_out - output);
    return 0;
}

int cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len) {
    if (input == NULL || output == NULL || out_len == NULL) {
        return 1;
    }

    const uint8_t *p_in = input;
    const uint8_t *p_in_end = input + in_len;
    uint8_t *p_out = output;
    uint8_t *p_out_end = output + *out_len;


    while (p_in < p_in_end) {
        uint8_t code = *p_in++;

        if (code == 0) break;

        if (p_in + code - 1 > p_in_end) return 1;

        for (uint8_t i = 1; i < code; i++) {
            if (p_out >= p_out_end) return 1;
            *p_out++ = *p_in++;
        }

        if (code < 0xFF && p_in < p_in_end && *p_in != 0x00) {
            if (p_out >= p_out_end) return 1;
            *p_out++ = 0x00;
        }
    }

    *out_len = (size_t)(p_out - output);

    return 0;
}

int calculate_crc16(const uint8_t *data, size_t length, uint16_t *crc_out) {
    if (data == NULL || crc_out == NULL) {
        return 1;
    }

    uint16_t crc = 0xFFFF;

    for (int i = 0; i<length; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC16_REFLECTED_POLY;
            } else {
                crc >>= 1;
            }
        }
    }
    *crc_out = crc;

    return 0;
}

int check_crc16(const uint8_t *data, size_t length, uint16_t expected_crc) {
    if (data == NULL) {
        return 1;
    }

    uint16_t calculated_crc;
    if (calculate_crc16(data, length, &calculated_crc) != 0) {
        return 1;
    }

    return (calculated_crc == expected_crc) ? 0 : 1;
}

int generate_encoded_message(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len, uint8_t *work_buffer, size_t work_len) {
    if (input == NULL || output == NULL || out_len == NULL) {
        return 1;
    }

    uint16_t crc;
    if (calculate_crc16(input, in_len, &crc) != 0) {
        return 1;
    }

    if (in_len + 2 > work_len) {
        return 1; // Not enough space for input data + CRC
    }

    memcpy(work_buffer, input, in_len);

    work_buffer[in_len] = (uint8_t)(crc & 0xFF); // Low byte
    work_buffer[in_len + 1] = (uint8_t)((crc >> 8) & 0xFF); // High byte

    const int result = cobs_encode(work_buffer,  in_len + 2, output, out_len);

    if (result != 0) {
        return result;
    }

    return 0;
}

int decode_message(const uint8_t *input, size_t in_len, uint8_t *output, size_t *out_len) {
    const int result = cobs_decode(input,  in_len, output, out_len);

    if (result != 0) {
        return result;
    }

    if (*out_len < 2) {
        return 1;
    }

    if (check_crc16(output, *out_len - 2, ((uint16_t)output[(*out_len) - 1] << 8) | output[(*out_len) - 2]) != 0) {
        return 1;
    }

    *out_len -= 2;

    return 0;
}