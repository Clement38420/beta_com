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

## Main API

The main API provides a simplified interface for generating and decoding messages with CRC16 and COBS.

### 1. Generate Encoded Message

This function automates the process of calculating the CRC16, appending it to the data, and then COBS-encoding the result.

```c
uint8_t raw_data[] = {0x01, 0x02, 0x03};
uint8_t encoded_message[256];
size_t encoded_len = sizeof(encoded_message);

// A work buffer is required to store the data + CRC before encoding.
// It must be at least sizeof(raw_data) + 2 bytes.
uint8_t work_buffer[sizeof(raw_data) + 2];

if (generate_encoded_message(raw_data, sizeof(raw_data), encoded_message, &encoded_len, work_buffer, sizeof(work_buffer)) == 0) {
    // encoded_message now contains the COBS-encoded frame with CRC
    // Ready to be sent over UART
}
```

### 2. Decode Message

This function decodes a COBS frame and verifies its integrity using the CRC16 checksum.

```c
uint8_t received_frame[] = {0x04, 0x01, 0x02, 0x03, 0x41, 0x73, 0x00}; // Example frame
uint8_t decoded_payload[256];
size_t decoded_len = sizeof(decoded_payload);

if (decode_message(received_frame, sizeof(received_frame), decoded_payload, &decoded_len) == 0) {
    // decoded_payload contains the original data {0x01, 0x02, 0x03}
    // CRC check was successful
}
```

## Points of Vigilance

1. **Buffer Initialization**: The `out_len` pointer passed to `cobs_encode` and `cobs_decode` serves a dual purpose. **Input**: Max buffer size. **Output**: Actual written size. Failing to initialize it with the buffer size will lead to errors (return code `1`) or buffer overflows.
2. **COBS Overhead**: The output buffer for `cobs_encode` must be larger than the input. Worst case overhead is 1 byte per 254 bytes of data, plus 1 overhead byte, plus 1 byte for the trailing zero.
3. **CRC Standard**: This library uses the **Modbus** standard (reflected polynomial `0xA001`). Ensure both the UART sender and receiver use this exact CRC configuration.
4. **Zero Delimiter**: `cobs_encode` automatically appends a `0x00` byte at the end of the frame. `cobs_decode` expects a valid COBS sequence.

## Error Codes

All functions return a `beta_com_err_t` value. A return value of `BETA_COM_SUCCESS` (0) indicates success. Any other value indicates an error.

| Code                          | Value | Description                                                              |
|-------------------------------|-------|--------------------------------------------------------------------------|
| `BETA_COM_SUCCESS`            | 0     | Operation successful.                                                    |
| `BETA_COM_ERR_INVALID_ARGS`   | -1    | A `NULL` pointer was passed for a required parameter.                    |
| `BETA_COM_ERR_BUFFER_TOO_SMALL` | -2    | The provided output or work buffer is not large enough for the result.   |
| `BETA_COM_ERR_DATA_CORRUPTED`   | -3    | The input data for `cobs_decode` is not a valid COBS-encoded sequence.   |
| `BETA_COM_ERR_CRC_MISMATCH`     | -4    | The CRC16 checksum of the decoded data does not match the expected value. |
| `BETA_COM_ERR_MSG_TOO_SHORT`    | -5    | The decoded message is too short to contain a valid CRC16 checksum.      |

