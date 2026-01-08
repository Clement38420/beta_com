#include "test_crc16.h"
#include "library.h"
#include <stdio.h>

void test_crc16() {
    printf("--- TEST CRC16 ---\n");

    struct {
        const uint8_t *data;
        size_t length;
        uint16_t expected_crc;
    } test_vectors[] = {
        {(const uint8_t *)"", 0, 0xFFFF},
        {(const uint8_t *)"H", 1, 0x76BF},
        {(const uint8_t *)"123456789", 9, 0x4B37},
        {(const uint8_t *)"Hello, World!", 13, 0x114E},
        {(const uint8_t *)"\x00\x01\x02\x03\x04\x05", 6, 0xA00E},
        {(const uint8_t *)"The quick brown fox jumps over the lazy dog", 43, 0xA89C},
    };

    size_t num_tests = sizeof(test_vectors) / sizeof(test_vectors[0]);
    for (size_t i = 0; i < num_tests; i++) {
        uint16_t crc;
        if (calculate_crc16(test_vectors[i].data, test_vectors[i].length, &crc) != 0) {
            printf("Test %zu: CRC calculation failed\n", i);
            continue;
        }
        if (check_crc16(test_vectors[i].data, test_vectors[i].length, test_vectors[i].expected_crc) != 0) {
            printf("Test %zu: Expected CRC 0x%04X, got 0x%04X\n", i, test_vectors[i].expected_crc, crc);
        } else {
            printf("Test %zu: Passed (CRC 0x%04X)\n", i, crc);
        }
    }

    printf("\n");
}
