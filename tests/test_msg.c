#include "test_msg.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "library.h"

void print_bytes(const uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", bytes[i]);
    }
}

void test_msg(void) {
    printf("--- TEST: ENCODE & DECODE LOOP ---\n");

    // Testing data
    const uint8_t original_data[] = {0x01, 0x02, 0x00, 0x03, 0xFF, 0xFE};
    size_t original_len = sizeof(original_data);

    // Buffers definition
    uint8_t work_buffer[128];
    uint8_t encoded_buffer[128];
    size_t encoded_len = sizeof(encoded_buffer);

    uint8_t decoded_buffer[128];
    size_t decoded_len = sizeof(decoded_buffer);

    printf("Data to encode : ");
    print_bytes(original_data, original_len);
    printf("\n\n");

    // Data encoding
    int enc_res = generate_encoded_message(
        original_data, original_len,
        encoded_buffer, &encoded_len,
        work_buffer, sizeof(work_buffer)
    );

    if (enc_res != 0) {
        printf("FAILED: Encoding returned error %d\n", enc_res);
        return;
    }

    printf("Encoded data : ");
    print_bytes(encoded_buffer, encoded_len);
    printf("\n");

    printf("Encoded size: %zu bytes\n\n", encoded_len);

    // Data decoding
    int dec_res = decode_message(
        encoded_buffer, encoded_len,
        decoded_buffer, &decoded_len
    );

    if (dec_res != 0) {
        printf("FAILED: Decoding returned error %d (CRC mismatch or format error)\n", dec_res);
        return;
    }

    printf("Decoded data : ");
    print_bytes(decoded_buffer, decoded_len);
    printf("\n");
    printf("Decoded size: %zu bytes\n\n", decoded_len);

    // Final comparaison
    if (decoded_len != original_len) {
        printf("FAILED: Size mismatch (Expected %zu, Got %zu)\n", original_len, decoded_len);
    } else if (memcmp(original_data, decoded_buffer, original_len) != 0) {
        printf("FAILED: Data mismatch (Content differs)\n");
    } else {
        printf("SUCCESS: Original data matches decoded data exactly.\n");
    }
    printf("\n");
}
