# beta_com Library

A lightweight C library designed for robust serial communication, providing high-level message handling, COBS encoding/decoding, CRC16 integrity checks, and ring-buffer-based stream processing.

## Features

*   **High-Level Message Handling**: Simple `send_message` and `receive_message` functions to handle packetization, encoding, and decoding automatically.
*   **Ring Buffers**: Integrated RX and TX ring buffers to manage asynchronous data streams efficiently.
*   **COBS Encoding/Decoding**: Frames data using `0x00` as a delimiter, ensuring the payload itself contains no zero bytes. Ideal for packet delineation over serial streams.
*   **CRC16 Integrity Check**: Implements CRC-16 according to the **Modbus** standard (reflected polynomial `0xA001`) to ensure data integrity.
*   **Zero-Copy IOVECs**: `cobs_encode` uses I/O vectors (`beta_iovec_t`) to encode data from multiple non-contiguous memory blocks without prior copying.
*   **CMake Support**: Easy integration via CMake with optional unit tests.

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

To build the unit tests, set the `BUILD_TESTS` option to `ON`:

```bash
cmake -DBUILD_TESTS=ON ..
make
# Run tests
./unit_tests
```

## High-Level API Usage

The primary way to use the library is through the `beta_com_handle_t`, which manages all the buffers and state.

### 1. Initialization

First, define the memory for the handle's buffers and initialize it.

```c
#include "beta_com.h"

// 1. Define storage for the handle's buffers
uint8_t rx_rb_storage[256];
uint8_t tx_rb_storage[256];
uint8_t rx_work_buff[256];
uint8_t tx_work_buff[256];

// 2. Create a configuration structure
beta_com_config_t config = {
    .rx_rb_storage = rx_rb_storage,
    .rx_rb_size = sizeof(rx_rb_storage),
    .rx_work_buff = rx_work_buff,
    .rx_work_buff_size = sizeof(rx_work_buff),
    .tx_rb_storage = tx_rb_storage,
    .tx_rb_size = sizeof(tx_rb_storage),
    .tx_work_buff = tx_work_buff,
    .tx_work_buff_size = sizeof(tx_work_buff),
};

// 3. Initialize the handle
beta_com_handle_t handle;
beta_com_err_t err = beta_com_init(&handle, &config);
if (err != BETA_COM_SUCCESS) {
    // Handle initialization error
}
```

### 2. Sending a Message

The `send_message` function automates CRC calculation, COBS encoding, and places the result in the TX ring buffer.

```c
const uint8_t payload[] = {0x01, 0x02, 0x03};

// The function handles CRC, COBS, and adds the final frame to the TX buffer.
int32_t bytes_sent = send_message(&handle, payload, sizeof(payload));

if (bytes_sent < 0) {
    // Handle error (e.g., buffer full)
}

// Now, you can read `bytes_sent` from `handle.tx_rb` and send them over UART.
// Example:
uint8_t byte_to_transmit;
while (rb_pop(&handle.tx_rb, &byte_to_transmit) == BETA_COM_SUCCESS) {
    // uart_send_byte(byte_to_transmit);
}
```

### 3. Receiving a Message

Push incoming bytes from your hardware (e.g., UART) into the RX ring buffer. Then, call `receive_message` to process the stream.

```c
// In your UART RX interrupt or polling loop:
// uint8_t received_byte = uart_read_byte();
// rb_push(&handle.rx_rb, received_byte);

// In your main loop, try to decode a message
uint8_t message_buffer[128];
int32_t message_len = receive_message(&handle, message_buffer, sizeof(message_buffer));

if (message_len > 0) {
    // A complete, valid message was received!
    // `message_buffer` contains the payload, and `message_len` is its size.
} else if (message_len == BETA_COM_ERR_NO_MESSAGE_FOUND) {
    // Not enough data yet to form a complete message.
} else {
    // An error occurred (e.g., CRC mismatch, buffer too small).
}
```

## Low-Level API

For advanced use cases, you can use the low-level functions directly.

### `cobs_encode`

Encodes data from one or more buffers (`beta_iovec_t`) into a COBS frame.

```c
uint8_t header[] = {0x01, 0x02};
uint8_t payload[] = {0x00, 0x03, 0x04};
uint8_t encoded_buffer[32];

beta_iovec_t buffers[] = {
    { .iov_base = header, .iov_len = sizeof(header) },
    { .iov_base = payload, .iov_len = sizeof(payload) }
};

// Returns the encoded length, including the final 0x00 delimiter.
int32_t encoded_len = cobs_encode(buffers, 2, encoded_buffer, sizeof(encoded_buffer));
```

### `cobs_decode`

Decodes a COBS frame back into its original data.

```c
// encoded_buffer from the example above
uint8_t decoded_buffer[32];
int32_t decoded_len = cobs_decode(encoded_buffer, encoded_len, decoded_buffer, sizeof(decoded_buffer));
```

### `calculate_crc16`

Calculates the CRC-16/Modbus checksum for a data buffer.

```c
uint8_t data[] = {0x01, 0x02, 0x03};
uint16_t crc = calculate_crc16(data, sizeof(data));
// crc now holds the calculated checksum.
```

## Error Codes

Functions returning an `int32_t` or `beta_com_err_t` will provide a status code. `BETA_COM_SUCCESS` (0) or a positive value (indicating length) means success.

| Code                          | Value | Description                                                              |
|-------------------------------|-------|--------------------------------------------------------------------------|
| `BETA_COM_SUCCESS`            | 0     | Operation successful.                                                    |
| `BETA_COM_ERR_INVALID_ARGS`   | -1    | A `NULL` pointer was passed for a required parameter.                    |
| `BETA_COM_ERR_BUFFER_TOO_SMALL` | -2    | The provided output or work buffer is not large enough for the result.   |
| `BETA_COM_ERR_INVALID_DATA`   | -3    | The input for `cobs_decode` is not a valid COBS-encoded sequence.        |
| `BETA_COM_ERR_CRC_MISMATCH`     | -4    | The CRC16 checksum of the decoded data does not match the expected value. |
| `BETA_COM_ERR_MSG_TOO_SHORT`    | -5    | The decoded message is too short to contain a valid CRC16 checksum.      |
| `BETA_COM_ERR_RB_FULL`          | -6    | The ring buffer is full.                                                 |
| `BETA_COM_ERR_RB_EMPTY`         | -7    | The ring buffer is empty.                                                |
| `BETA_COM_ERR_NO_MESSAGE_FOUND` | -8    | No complete message (ending in 0x00) was found in the ring buffer.       |
| `BETA_COM_ERR_RB_NOT_ENOUGH_SPACE` | -9 | Not enough space in the ring buffer to push the entire message.          |

