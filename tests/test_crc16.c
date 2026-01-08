#include "test_crc16.h"
#include "library.h"
#include <stdio.h>

void test_crc16(void) {
    printf("--- TEST: CRC16 CALCULATION & VERIFICATION ---\n");

    // Test vectors
    struct {
        const char *name;
        const uint8_t *data;
        size_t length;
        uint16_t expected_crc;
    } test_vectors[] = {
        {"Empty string", (const uint8_t *)"", 0, 0xFFFF},
        {"Single char 'H'", (const uint8_t *)"H", 1, 0x76BF},
        {"ASCII numbers", (const uint8_t *)"123456789", 9, 0x4B37},
        {"Hello, World!", (const uint8_t *)"Hello, World!", 13, 0x114E},
        {"Binary data", (const uint8_t *)"\x00\x01\x02\x03\x04\x05", 6, 0xA00E},
        {"Long sentence", (const uint8_t *)"The quick brown fox jumps over the lazy dog", 43, 0xA89C},
    };

    size_t num_tests = sizeof(test_vectors) / sizeof(test_vectors[0]);
    int all_passed = 1;

    for (size_t i = 0; i < num_tests; i++) {
        uint16_t calculated_crc;
        beta_com_err_t calc_res = calculate_crc16(test_vectors[i].data, test_vectors[i].length, &calculated_crc);

        if (calc_res != BETA_COM_SUCCESS) {
            printf("FAILED [%s]: CRC calculation returned error %d\n", test_vectors[i].name, calc_res);
            all_passed = 0;
            continue;
        }

        if (calculated_crc != test_vectors[i].expected_crc) {
            printf("FAILED [%s]: CRC mismatch (Expected 0x%04X, Got 0x%04X)\n", test_vectors[i].name, test_vectors[i].expected_crc, calculated_crc);
            all_passed = 0;
        } else {
            beta_com_err_t check_res = check_crc16(test_vectors[i].data, test_vectors[i].length, test_vectors[i].expected_crc);
            if (check_res != BETA_COM_SUCCESS) {
                printf("FAILED [%s]: CRC check failed with error %d\n", test_vectors[i].name, check_res);
                all_passed = 0;
            } else {
                printf("SUCCESS [%s]: CRC 0x%04X matches expected value.\n", test_vectors[i].name, calculated_crc);
            }
        }
    }

    if (all_passed) {
        printf("\nAll CRC16 tests passed successfully.\n");
    } else {
        printf("\nSome CRC16 tests failed.\n");
    }
    printf("\n");
}



