#include <assert.h>
#include <string.h>
#include "../beta_com.h"

#include "test_cobs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../beta_com.h"

// Helper function to print byte arrays
static void print_bytes(const char* prefix, const uint8_t *bytes, size_t len) {
    printf("%s[%zu bytes]: ", prefix, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", bytes[i]);
    }
    printf("\n");
}

void test_cobs_suite(void) {
    printf("--- TEST: COBS Encoding and Decoding ---\n");

    // A tricky test case with consecutive zeros
    uint8_t raw[] = {0x11, 0x22, 0x00, 0x33, 0x00, 0x00, 0x44};
    uint8_t encoded[32];
    uint8_t decoded[32];

    print_bytes("Original data ", raw, sizeof(raw));

    // 1. Encoding
    printf("\nStep 1: Encoding...\n");
    beta_iovec_t vec = { .iov_base = raw, .iov_len = sizeof(raw) };
    int32_t enc_len = cobs_encode(&vec, 1, encoded, sizeof(encoded));
    assert(enc_len > 0);
    print_bytes("Encoded data  ", encoded, enc_len);

    // The encoded message must end with a zero byte
    assert(encoded[enc_len - 1] == 0x00);

    // Verify that there are no zero bytes in the message body
    for(int i = 0; i < enc_len - 1; i++) {
        assert(encoded[i] != 0x00);
    }
    printf("  - Verified: No zero bytes in the encoded body.\n");
    printf("  - Verified: Message ends with a zero byte.\n");

    // 2. Decoding
    printf("\nStep 2: Decoding...\n");
    int32_t dec_len = cobs_decode(encoded, enc_len, decoded, sizeof(decoded));
    assert(dec_len > 0);
    print_bytes("Decoded data  ", decoded, dec_len);

    // 3. Verification
    printf("\nStep 3: Verifying...\n");
    assert(dec_len == sizeof(raw));
    assert(memcmp(raw, decoded, dec_len) == 0);
    printf("SUCCESS: Decoded data matches original data.\n\n");
}