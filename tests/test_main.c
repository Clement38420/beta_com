#include "test_cobs.h"
#include "test_crc16.h"
#include "test_msg.h"

int main(void) {
    test_cobs();
    test_crc16();
    test_msg();

    return 0;
}
