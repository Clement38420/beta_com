#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "library.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int test_cobs_encode_decode(void) {
    // 1. Test data (mix of values and zeros)
    uint8_t original[] = {0x11, 0x00, 0x22, 0x33, 0x00, 0x44};
    size_t original_len = sizeof(original);

    uint8_t encoded[256];
    size_t encoded_len = sizeof(encoded);

    uint8_t decoded[256];
    size_t decoded_len = sizeof(decoded);

    printf("--- COBS TEST ---\n");
    print_hex("Original", original, original_len);

    // 2. ENCODE test
    if (cobs_encode(original, original_len, encoded, &encoded_len) != 0) {
        printf("Error: Encoding failed\n");
        return 1;
    }
    print_hex("Encoded ", encoded, encoded_len);

    // Basic COBS format check: the last byte must be 0x00
    assert(encoded[encoded_len - 1] == 0x00);

    // 3. DECODE test
    // Note: Pass encoded_len - 1 to exclude the final 0x00 from decoding
    // if your function stops at the end pointer.
    if (cobs_decode(encoded, encoded_len - 1, decoded, &decoded_len) != 0) {
        printf("Error: Decoding failed\n");
        return 1;
    }
    print_hex("Decoded ", decoded, decoded_len);

    // 4. Verify that decoded data matches the original data
    if (original_len == decoded_len && memcmp(original, decoded, original_len) == 0) {
        printf("\nRESULT: SUCCESS (Data are identical)\n");
    } else {
        printf("\nRESULT: FAILURE (Data differ)\n");
        return 1;
    }

    return 0;
}
