#include <assert.h>
#include <string.h>
#include "../beta_com.h"

#include "test_crc16.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../beta_com.h"

void test_crc16_suite(void) {
    printf("--- TEST: CRC-16/MODBUS Calculation ---\n");

    // Standard test vector for CRC-16/MODBUS (Polynomial 0xA001)
    // Input: "123456789" -> Expected output: 0x4B37
    const uint8_t data[] = "123456789";
    printf("Test vector: \"123456789\"\n");
    uint16_t crc = calculate_crc16(data, sizeof(data) - 1);
    printf("  - Calculated CRC: 0x%04X\n", crc);
    printf("  - Expected CRC:   0x4B37\n");
    assert(crc == 0x4B37);
    printf("  SUCCESS: CRC matches expected value.\n\n");

    // Test with an empty buffer
    printf("Test vector: Empty data\n");
    crc = calculate_crc16(NULL, 0);
    printf("  - Calculated CRC: 0x%04X\n", crc);
    printf("  - Expected CRC:   0x0000\n");
    assert(crc == 0);
    printf("  SUCCESS: CRC for NULL data is 0.\n\n");
}