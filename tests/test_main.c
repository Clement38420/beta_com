#include <stdio.h>
#include "test_crc16.h"
#include "test_cobs.h"
#include "test_msg.h"

int main(void) {
    printf("=== RUNNING BETA_COM UNIT TESTS ===\n\n");

    test_crc16_suite();
    test_cobs_suite();
    test_msg_loopback();

    printf("\n=== ALL TESTS PASSED SUCCESSFULLY ===\n");
    return 0;
}