# beta_com Library

A lightweight C library designed for **UART communication**, providing **COBS (Consistent Overhead Byte Stuffing)** encoding/decoding and **CRC16** checksum calculation.

## Features

* **COBS Encoding/Decoding**: Frames data using `0x00` as a delimiter, ensuring the payload itself contains no zero bytes. Ideal for packet delineation over serial streams.
* **CRC16 Integrity Check**: Implements CRC-16 according to the **Modbus** standard (reflected polynomial `0xA001`).
* **CMake Support**: Easy integration via CMake with optional unit tests.

## Build Instructions

This project uses CMake. You can build the library and optionally the tests.

### Building the Library
```bash
mkdir build
cd build
cmake ..
make

```

### Building Unit Tests

To build the unit tests, set the `BUILD_TESTS` option to `ON` (default is `OFF`):

```bash
cmake -DBUILD_TESTS=ON ..
make
# Run tests
./unit_tests

```

## Usage

Include the header in your application:

```c
#include "library.h"

```

### 1. COBS Encoding & Decoding

**Important:** You must initialize the `out_len` variable with the **maximum capacity** of your output buffer before calling the functions. Upon return, it will contain the actual length of the processed data.

```c
uint8_t raw_data[] = {0x11, 0x00, 0x22};
uint8_t encoded[256];
size_t encoded_len = sizeof(encoded); // MUST initialize with buffer capacity

// Encoding
if (cobs_encode(raw_data, sizeof(raw_data), encoded, &encoded_len) == 0) {
    // encoded now contains the COBS frame (ending with 0x00)
    // Ready to be sent over UART
}

// Decoding
uint8_t decoded[256];
size_t decoded_len = sizeof(decoded); // MUST initialize with buffer capacity

// Note: Decoding usually processes data up to the delimiter.
if (cobs_decode(encoded, encoded_len, decoded, &decoded_len) == 0) {
    // decoded contains {0x11, 0x00, 0x22}
}

```

### 2. CRC16 Calculation (Modbus)

Calculates a 16-bit checksum to ensure data integrity using the Modbus standard.

```c
uint8_t data[] = {0x01, 0x02, 0x03};
uint16_t crc;

// Calculate
if (calculate_crc16(data, sizeof(data), &crc) == 0) {
    // crc contains the Modbus checksum
}

// Verify
if (check_crc16(data, sizeof(data), expected_crc_val) == 0) {
    // Data is valid
}

```

## Points of Vigilance

1. **Buffer Initialization**: The `out_len` pointer passed to `cobs_encode` and `cobs_decode` serves a dual purpose. **Input**: Max buffer size. **Output**: Actual written size. Failing to initialize it with the buffer size will lead to errors (return code `1`) or buffer overflows.
2. **COBS Overhead**: The output buffer for `cobs_encode` must be larger than the input. Worst case overhead is 1 byte per 254 bytes of data, plus 1 overhead byte, plus 1 byte for the trailing zero.
3. **CRC Standard**: This library uses the **Modbus** standard (reflected polynomial `0xA001`). Ensure both the UART sender and receiver use this exact CRC configuration.
4. **Zero Delimiter**: `cobs_encode` automatically appends a `0x00` byte at the end of the frame. `cobs_decode` expects a valid COBS sequence.