#ifndef BETACOM_LIBRARY_H
#define BETACOM_LIBRARY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <atomic>
using atomic_size_t = std::atomic<size_t>;
#else
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CRC16_REFLECTED_POLY 0xA001
#define BETA_COM_CALC_WORK_SIZE(payload_size) ((payload_size) + 2 + ((payload_size + 2) / 254) + 2) // PAYLAOD + 2 CRC bytes + COBS overhead + 1 trailing 0x00
#define BETA_COM_RING_BUFFER_MULTIPLIER 4 // Multiplier for ring-buffer size relative to a full size message

typedef enum {
    BETA_COM_SUCCESS = 0,                    // Operation successful
    BETA_COM_ERR_INVALID_ARGS = -1,          // NULL pointers passed as parameters
    BETA_COM_ERR_BUFFER_TOO_SMALL = -2,      // Output or work buffer too small
    BETA_COM_ERR_INVALID_DATA = -3,          // General error for invalid data
    BETA_COM_ERR_CRC_MISMATCH = -4,          // Calculated CRC does not match received CRC
    BETA_COM_ERR_MSG_TOO_SHORT = -5,         // Decoded message too short to contain a CRC
    BETA_COM_ERR_RB_FULL = -6,               // Full ring-buffer
    BETA_COM_ERR_RB_EMPTY = -7,              // Empty ring-buffer
    BETA_COM_ERR_NO_MESSAGE_FOUND = -8,      // No complete message found in the ring-buffer
    BETA_COM_ERR_RB_NOT_ENOUGH_SPACE = -9,   // Not enough space in the ring-buffer to push data
    BETA_COM_ERR_OUT_OF_MEMORY = -10,        // Dynamic memory allocation failed
    BETA_COM_ERR_RB_NOT_ENOUGH_DATA = -11,   // Not enough data in the ring-buffer to read
} beta_com_err_t;

typedef struct {
    uint8_t *iov_base;
    size_t iov_len;
} beta_iovec_t;

typedef struct {
    uint8_t *buffer;
    atomic_size_t head;
    atomic_size_t tail;
    size_t max_size;
} ring_buffer_t;

/**
 * @brief Initializes a ring buffer.
 *
 * @param rb Pointer to the ring buffer structure to initialize.
 * @param storage_array Pointer to the memory buffer to be used by the ring buffer.
 * @param size The size of the storage_array.
 * @return BETA_COM_SUCCESS on success, or BETA_COM_ERR_INVALID_ARGS if rb or storage_array is NULL.
 */
beta_com_err_t rb_init(ring_buffer_t *rb, uint8_t *storage_array, size_t size);

/**
 * @brief Pushes a single byte into the ring buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @param data The byte to be pushed into the buffer.
 * @return BETA_COM_SUCCESS on success, BETA_COM_ERR_RB_FULL if the buffer is full, or BETA_COM_ERR_INVALID_ARGS if rb is NULL.
 */
beta_com_err_t rb_push(ring_buffer_t *rb, uint8_t data);

/**
 * @brief Pops a single byte from the ring buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @param data Pointer to a variable where the popped byte will be stored.
 * @return BETA_COM_SUCCESS on success, BETA_COM_ERR_RB_EMPTY if the buffer is empty, or BETA_COM_ERR_INVALID_ARGS if rb or data is NULL.
 */
beta_com_err_t rb_pop(ring_buffer_t *rb, uint8_t *data);

/**
 * @brief Returns the number of bytes currently stored in the ring buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @return The number of bytes available to be read from the buffer.
 */
size_t rb_free_size(const ring_buffer_t *rb);

/**
 * @brief Returns the number of bytes currently used in the ring buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @return The number of bytes currently stored in the buffer.
 */
size_t rb_used_size(const ring_buffer_t *rb);

/**
 * @brief Reads a linear block of contiguous data from the ring buffer and write it into a given buffer.
 *
 * This function is useful for reading data that potentially wraps around the end of the buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @param buff Pointer to a buff that will receive the data.
 * @param block_size Block size to read.
 * @return BETA_COM_SUCCESS on success, or BETA_COM_ERR_INVALID_ARGS if rb or block_ptr is NULL.
 */
beta_com_err_t rb_read_linear_block(ring_buffer_t *rb, uint8_t *buff, size_t block_size);

/**
 * @brief Writes a linear block of contiguous data into the ring buffer from a given buffer.
 *
 * This function is useful for writing data that potentially wraps around the end of the buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @param buff Pointer to a buff that contains the data to write.
 * @param block_size Block size to write.
 * @return BETA_COM_SUCCESS on success, or BETA_COM_ERR_INVALID_ARGS if rb or buff is NULL.
 */
beta_com_err_t rb_write_linear_block(ring_buffer_t *rb, const uint8_t *buff, size_t block_size);

/**
 * @brief Searches for the first occurrence of a specific byte in the ring buffer.
 *
 * @param rb Pointer to the ring buffer.
 * @param byte The byte to search for.
 * @return A pointer to the first occurrence of the byte within the buffer's linear memory, or NULL if the byte is not found.
 */
uint8_t* rbchr(const ring_buffer_t *rb, uint8_t byte);

typedef struct {
    ring_buffer_t rx_rb;
    uint8_t *rx_work_buff;
    size_t rx_wb_size;
    size_t rx_search_from;

    ring_buffer_t tx_rb;
    uint8_t *tx_work_buff;
    size_t tx_wb_size;

    bool is_dynamic;
} beta_com_handle_t;

typedef struct {
    uint8_t *rx_rb_storage;
    size_t rx_rb_size;
    uint8_t *rx_work_buff;
    size_t rx_work_buff_size;
    uint8_t *tx_rb_storage;
    size_t tx_rb_size;
    uint8_t *tx_work_buff;
    size_t tx_work_buff_size;
    bool use_dynamic_alloc;
} beta_com_config_t;

/**
 * @brief Initializes a beta_com handle with the provided configuration.
 *
 * @param handle Pointer to the beta_com handle to initialize.
 * @param config Pointer to the configuration structure containing buffer pointers and sizes.
 * @return BETA_COM_SUCCESS on success, or an error code on failure.
 */
beta_com_err_t beta_com_init(beta_com_handle_t *handle, const beta_com_config_t *config);

/**
 * @brief Initializes a beta_com handle with automatic buffer allocation.
 *
 * This function allocates necessary buffers based on the specified maximum payload size.
 *
 * @param handle Pointer to the beta_com handle to initialize.
 * @param max_payload_size Maximum expected payload size for messages.
 * @return BETA_COM_SUCCESS on success, or an error code on failure.
 */
beta_com_err_t beta_com_init_easy(beta_com_handle_t *handle, size_t max_payload_size);

/**
 * @brief Deinitializes a beta_com handle and frees allocated resources.
 *
 * @param handle Pointer to the beta_com handle to deinitialize.
 */
void beta_com_deinit(beta_com_handle_t *handle);

/**
 * @brief Encodes a byte buffer using the COBS algorithm.
 *
 * This function encodes the input data and automatically appends the
 * packet delimiter (0x00) at the end of the output buffer.
 *
 * @param buffers     Buffers to encode
 * @param buffers_count        Number of buffers
 * @param output        Pointer to the destination buffer.
 * @param max_out_len   Maximum capacity of the destination buffer.
 * * @return Total number of bytes written to output (including the trailing 0x00). Or an error code < 0.
 */
int32_t cobs_encode(const beta_iovec_t *buffers, size_t buffers_count, uint8_t *output, size_t max_out_len);

/**
 * @brief Decodes a COBS-encoded frame.
 *
 * Reconstructs the original data from a COBS frame. Stops decoding if a
 * zero delimiter is encountered or the input length is reached.
 *
 * @param input         Pointer to the COBS-encoded data.
 * @param in_len        Length of the encoded data.
 * @param output        Pointer to the destination buffer.
 * @param max_out_len   Maximum capacity of the destination buffer.
 * * @return Length of the decoded payload.. Or an error code < 0.
 */
int32_t cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t max_out_len);

/**
 * @brief Calculates a CRC-16 checksum (Modbus/Reflected).
 *
 * Uses polynomial 0xA001 with an initial value of 0xFFFF.
 *
 * @param data      Pointer to the data buffer.
 * @param length    Length of the data buffer.
 * * @return uint16_t: The calculated CRC16 value. Returns 0 if data is NULL.
 */
uint16_t calculate_crc16(const uint8_t *data, size_t length);

/**
 * @brief Receives a complete message from the communication handle.
 *
 * This function processes the incoming byte stream, decodes a COBS frame,
 * verifies its CRC, and returns the payload.
 *
 * @param handle Pointer to the beta_com handle.
 * @param buff Pointer to the buffer where the received message payload will be stored.
 * @param buff_size The maximum size of the receive buffer.
 * @return The length of the received message payload on success, or a negative error code on failure.
 */
int32_t receive_message(beta_com_handle_t *handle, uint8_t *buff, size_t buff_size);

/**
 * @brief Sends a message through the communication handle.
 *
 * This function takes a payload, calculates its CRC, COBS-encodes it,
 * and places it into the transmission buffer.
 *
 * @param handle Pointer to the beta_com handle.
 * @param buff Pointer to the message payload to send.
 * @param buff_size The length of the message payload.
 * @return The number of bytes written to the transmission buffer on success, or a negative error code on failure.
 */
int32_t send_message(beta_com_handle_t *handle, const uint8_t *buff, size_t buff_size);

#ifdef __cplusplus
}
#endif

#endif // BETACOM_LIBRARY_H