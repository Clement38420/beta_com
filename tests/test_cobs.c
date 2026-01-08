#include "test_cobs.h"

#include <stdio.h>
#include <string.h>

#include "library.h"

static void print_bytes(const uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", bytes[i]);
    }
}

void test_cobs(void) {
    printf("--- TEST: COBS ENCODE & DECODE ---\n");

    // Test data
    const uint8_t original_data[] = {0x11, 0x00, 0x22, 0x33, 0x00, 0x44};
    size_t original_len = sizeof(original_data);

    // Buffers
    uint8_t encoded_buffer[128];
    size_t encoded_len = sizeof(encoded_buffer);
    uint8_t decoded_buffer[128];
    size_t decoded_len = sizeof(decoded_buffer);

    printf("Data to encode : ");
    print_bytes(original_data, original_len);
    printf("\n\n");

    // Encode
    beta_com_err_t enc_res = cobs_encode(original_data, original_len, encoded_buffer, &encoded_len);
    if (enc_res != BETA_COM_SUCCESS) {
        printf("FAILED: Encoding returned error %d\n", enc_res);
        return;
    }

    printf("Encoded data : ");
    print_bytes(encoded_buffer, encoded_len);
    printf("\n");
    printf("Encoded size: %zu bytes\n\n", encoded_len);

    // Decode
    beta_com_err_t dec_res = cobs_decode(encoded_buffer, encoded_len, decoded_buffer, &decoded_len);
    if (dec_res != BETA_COM_SUCCESS) {
        printf("FAILED: Decoding returned error %d\n", dec_res);
        return;
    }

    printf("Decoded data : ");
    print_bytes(decoded_buffer, decoded_len);
    printf("\n");
    printf("Decoded size: %zu bytes\n\n", decoded_len);

    // Final comparison
    if (decoded_len != original_len) {
        printf("FAILED: Size mismatch (Expected %zu, Got %zu)\n", original_len, decoded_len);
    } else if (memcmp(original_data, decoded_buffer, original_len) != 0) {
        printf("FAILED: Data mismatch (Content differs)\n");
    } else {
        printf("SUCCESS: Original data matches decoded data exactly.\n");
    }
    printf("\n");
}



