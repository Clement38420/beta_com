#include "test_cobs.h"
#include "test_crc16.h"

int main(void) {
    test_cobs_encode_decode();
    test_crc16();

    return 0;
}
