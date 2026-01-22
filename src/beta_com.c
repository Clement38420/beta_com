#include "../include/beta_com.h"

#include <stdlib.h>
#include <string.h>

beta_com_err_t beta_com_init(beta_com_handle_t *handle, const beta_com_config_t *config) {
    if (handle == NULL || config == NULL) return BETA_COM_ERR_INVALID_ARGS;

    // Dynamic allocation mode: malloc all buffers
    if (config->use_dynamic_alloc == true) {
        handle->is_dynamic = true;

        int err_code = 0;
        handle->tx_work_buff = malloc(config->tx_work_buff_size);
        err_code |= (handle->tx_work_buff == NULL);
        handle->rx_work_buff = malloc(config->rx_work_buff_size);
        err_code |= (handle->rx_work_buff == NULL);
        handle->tx_rb.buffer = malloc(config->tx_rb_size);
        err_code |= (handle->tx_rb.buffer == NULL);
        handle->rx_rb.buffer = malloc(config->rx_rb_size);
        err_code |= (handle->rx_rb.buffer == NULL);

        // If any allocation fails, free all and return an error
        if (err_code != 0) {
            free(handle->tx_work_buff);
            free(handle->rx_work_buff);
            free(handle->tx_rb.buffer);
            free(handle->rx_rb.buffer);
            return BETA_COM_ERR_OUT_OF_MEMORY;
        }

        handle->tx_wb_size = config->tx_work_buff_size;
        handle->rx_wb_size = config->rx_work_buff_size;

        rb_init(&handle->rx_rb, handle->rx_rb.buffer, config->rx_rb_size);
        handle->rx_search_from = atomic_load(&handle->rx_rb.tail);
        rb_init(&handle->tx_rb, handle->tx_rb.buffer, config->tx_rb_size);

        return BETA_COM_SUCCESS;
    }

    // Static allocation mode: use user-provided buffers
    if (config->rx_rb_storage == NULL) return BETA_COM_ERR_INVALID_ARGS;
    rb_init(&handle->rx_rb, config->rx_rb_storage, config->rx_rb_size);

    if (config->rx_work_buff == NULL) return BETA_COM_ERR_INVALID_ARGS;
    handle->rx_work_buff = config->rx_work_buff;
    handle->rx_wb_size = config->rx_work_buff_size;

    if (config->tx_rb_storage == NULL) return BETA_COM_ERR_INVALID_ARGS;
    rb_init(&handle->tx_rb, config->tx_rb_storage, config->tx_rb_size);

    if (config->tx_work_buff == NULL) return BETA_COM_ERR_INVALID_ARGS;
    handle->tx_work_buff = config->tx_work_buff;
    handle->tx_wb_size = config->tx_work_buff_size;
    return BETA_COM_SUCCESS;
}

beta_com_err_t beta_com_init_easy(beta_com_handle_t *handle, size_t max_payload_size) {
    if (max_payload_size == 0) return BETA_COM_ERR_INVALID_ARGS;

    // Automatically calculate required buffer sizes based on max payload
    size_t work_buff_size = BETA_COM_CALC_WORK_SIZE(max_payload_size);
    size_t rb_size = work_buff_size * BETA_COM_RING_BUFFER_MULTIPLIER;

    beta_com_config_t auto_conf = {
        .use_dynamic_alloc = 1,
        .rx_work_buff_size = work_buff_size,
        .tx_work_buff_size = work_buff_size,
        .rx_rb_size = rb_size,
        .tx_rb_size = rb_size,
    };

    return beta_com_init(handle, &auto_conf);
}

void beta_com_deinit(beta_com_handle_t *handle) {
    if (handle == NULL) return;

    // Free memory only if it was dynamically allocated
    if (handle->is_dynamic) {
        free(handle->tx_work_buff);
        free(handle->rx_work_buff);
        free(handle->tx_rb.buffer);
        free(handle->rx_rb.buffer);
    }

    // Clear the handle to prevent dangling pointers
    memset(handle, 0, sizeof(beta_com_handle_t));
}

int32_t cobs_encode(const beta_iovec_t *buffers, size_t buffers_count, uint8_t *output, size_t max_out_len) {
    if (buffers == NULL || output == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    uint8_t *p_out = output;
    uint8_t *p_out_end = output + max_out_len;
    if (max_out_len < 2) return BETA_COM_ERR_BUFFER_TOO_SMALL;

    uint8_t *p_code = p_out++; // Reserve space for the first code byte
    uint8_t code = 1;

    // Process each input buffer
    for (size_t i = 0; i < buffers_count; i++) {
        const uint8_t *p_in = buffers[i].iov_base;
        const uint8_t *p_in_end = buffers[i].iov_base + buffers[i].iov_len;

        while (p_in < p_in_end) {
            if (*p_in == 0) {
                // Zero byte found, finalize the current block
                *p_code = code;
                if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                p_code = p_out++;
                code = 1;
            } else {
                // Copy non-zero byte
                if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                *p_out++ = *p_in;
                code++;

                if (code == 0xFF) {
                    // Maximum code value reached, start a new block
                    *p_code = code;
                    if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                    p_code = p_out++;
                    code = 1;
                }
            }
            p_in++;
        }
    }

    // Finalize the last block
    *p_code = code;

    if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
    *p_out++ = 0x00; // Add the trailing zero byte to mark end of frame

    return (int32_t)(p_out - output);
}

int32_t cobs_decode(const uint8_t *input, size_t in_len, uint8_t *output, size_t max_out_len) {
    if (input == NULL || output == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    const uint8_t *p_in = input;
    const uint8_t *p_in_end = input + in_len;
    uint8_t *p_out = output;
    uint8_t *p_out_end = output + max_out_len;

    while (p_in < p_in_end) {
        uint8_t code = *p_in++;

        if (code == 0) break; // End of frame

        // Check for invalid data that would read past the input buffer
        if (p_in + code - 1 > p_in_end) return BETA_COM_ERR_INVALID_DATA;

        // Copy the data block
        for (uint8_t i = 1; i < code; i++) {
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            *p_out++ = *p_in++;
        }

        // Re-insert a zero byte, unless it's a 0xFF block (which doesn't represent a zero)
        if (code < 0xFF && p_in < p_in_end && *p_in != 0x00) {
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            *p_out++ = 0x00;
        }
    }

    return (int32_t)(p_out - output);
}

uint16_t calculate_crc16(const uint8_t *data, size_t length) {
    if (data == NULL) {
        return 0;
    }

    uint16_t crc = 0xFFFF; // Initial value for CRC-16/MODBUS

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            // Use reflected polynomial 0xA001
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC16_REFLECTED_POLY;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

beta_com_err_t rb_init(ring_buffer_t *rb, uint8_t *storage_array, size_t size) {
    if (rb == NULL || storage_array == NULL) return BETA_COM_ERR_INVALID_ARGS;

    rb->buffer = storage_array;
    rb->head = 0;
    rb->tail = 0;
    rb->max_size = size;
    return BETA_COM_SUCCESS;
}

// This function is ISR-safe due to atomic operations
beta_com_err_t rb_push(ring_buffer_t *rb, uint8_t data) {
    size_t t = atomic_load(&rb->tail);
    size_t h = atomic_load(&rb->head);

    size_t next_h = (h + 1) % rb->max_size;

    if (next_h == t) return BETA_COM_ERR_RB_FULL; // Buffer is full

    rb->buffer[h] = data;
    atomic_store(&rb->head, next_h);
    return BETA_COM_SUCCESS;
}

// This function is ISR-safe due to atomic operations
beta_com_err_t rb_pop(ring_buffer_t *rb, uint8_t *data) {
    size_t t = atomic_load(&rb->tail);
    size_t h = atomic_load(&rb->head);

    if (t == h) return BETA_COM_ERR_RB_EMPTY; // Buffer is empty

    *data = rb->buffer[rb->tail];
    atomic_store(&rb->tail, (t + 1) % rb->max_size);
    return BETA_COM_SUCCESS;
}

size_t rb_free_size(const ring_buffer_t *rb) {
    size_t t = atomic_load(&rb->tail);
    size_t h = atomic_load(&rb->head);
    return t > h ? t - h - 1 : t + rb->max_size - h - 1;
}

size_t rb_used_size(const ring_buffer_t *rb) {
    size_t t = atomic_load(&rb->tail);
    size_t h = atomic_load(&rb->head);
    return h >= t ? h - t : h + rb->max_size - t;
}

beta_com_err_t rb_read_linear_block(ring_buffer_t *rb, uint8_t *buff, size_t block_size) {
    if (rb == NULL || buff == NULL) return BETA_COM_ERR_INVALID_ARGS;

    size_t t = atomic_load(&rb->tail);

    if (rb_used_size(rb) < block_size) {
        return BETA_COM_ERR_RB_NOT_ENOUGH_DATA;
    }

    // Handle wrap-around case where data crosses the buffer boundary
    if (t + block_size <= rb->max_size) {
        memcpy(buff, &(rb->buffer[t]), block_size);
    } else {
        size_t first_part_size = rb->max_size - t;
        memcpy(buff, &(rb->buffer[t]), first_part_size);
        memcpy(buff + first_part_size, &(rb->buffer[0]), block_size - first_part_size);
    }

    atomic_store(&rb->tail, (t + block_size) % rb->max_size);

    return BETA_COM_SUCCESS;
}

beta_com_err_t rb_write_linear_block(ring_buffer_t *rb, const uint8_t *buff, size_t block_size) {
    if (rb == NULL || buff == NULL) return BETA_COM_ERR_INVALID_ARGS;

    size_t h = atomic_load(&rb->head);

    if (rb_free_size(rb) < block_size) {
        return BETA_COM_ERR_RB_NOT_ENOUGH_SPACE;
    }

    // Handle wrap-around case where data crosses the buffer boundary
    if (h + block_size <= rb->max_size) {
        memcpy(&(rb->buffer[h]), buff, block_size);
    } else {
        size_t first_part_size = rb->max_size - h;
        memcpy(&(rb->buffer[h]), buff, first_part_size);
        memcpy(&(rb->buffer[0]), buff + first_part_size, block_size - first_part_size);
    }

    atomic_store(&rb->head, (h + block_size) % rb->max_size);

    return BETA_COM_SUCCESS;
}

uint8_t* rbchr(const ring_buffer_t *rb, uint8_t byte) {
    size_t t = atomic_load(&rb->tail);
    size_t h = atomic_load(&rb->head);

    // Search from tail to head, handling wrap-around
    for (size_t i = t; i != h; ) {
        if (rb->buffer[i] == byte) return &(rb->buffer[i]);
        i = (i + 1) % rb->max_size;
    }
    return NULL;
}

beta_com_err_t rb_flush(ring_buffer_t *rb) {
    if (rb == NULL) return BETA_COM_ERR_INVALID_ARGS;

    atomic_store(&rb->tail, atomic_load(&rb->head));
    return BETA_COM_SUCCESS;
}

int32_t receive_message(beta_com_handle_t *handle, uint8_t *buff, size_t buff_size) {
    size_t tail = atomic_load(&handle->rx_rb.tail);
    size_t head = atomic_load(&handle->rx_rb.head);

    // Find the next message delimiter (0x00) in the ring buffer
    // Search from previous search finish index to head, handling wrap-around
    if ((tail < head && (handle->rx_search_from > head || handle->rx_search_from < tail)) ||
        (tail > head && (handle->rx_search_from > head && handle->rx_search_from < tail))) {
        handle->rx_search_from = tail; // Reset search_from to tail if the search index is out of bounds
    }

    size_t i = handle->rx_search_from;
    size_t end = i;
    while (i != head) {
        if (handle->rx_rb.buffer[i] == 0x00) {
            end = i;
            break;
        }
        i = (i + 1) % handle->rx_rb.max_size;
    }
    handle->rx_search_from = i; // Update search_from for next call
    if (i == head) {
        if (rb_free_size(&handle->rx_rb) == 0) {
            // Buffer is full but no message delimiter found, flush to recover
            rb_flush(&handle->rx_rb);
            handle->rx_search_from = atomic_load(&handle->rx_rb.tail);
        }
        return BETA_COM_ERR_NO_MESSAGE_FOUND;
    }

    // Calculate the full message size, accounting for buffer wrap-around
    size_t msg_size = end < tail ? end + handle->rx_rb.max_size - tail + 1 : end - tail + 1;

    if (msg_size > handle->rx_wb_size) {
        // Message is too large for the work buffer, discard it and advance tail
        size_t next_tail = (end + 1) % handle->rx_rb.max_size;
        atomic_store(&handle->rx_rb.tail, next_tail);
        handle->rx_search_from = atomic_load(&handle->rx_rb.tail); // Reset search_from to the new tail after successful read
        return BETA_COM_ERR_BUFFER_TOO_SMALL;
    }
    // Read the encoded message from the ring buffer
    beta_com_err_t rb_read_err = rb_read_linear_block(&handle->rx_rb, handle->rx_work_buff, msg_size);
    if (rb_read_err != 0) {
        return rb_read_err;
    }
    handle->rx_search_from = atomic_load(&handle->rx_rb.tail); // Reset search_from to the new tail after successful read

    // Decode the message from the work buffer into the final output buffer
    int32_t decoded_len = cobs_decode(handle->rx_work_buff,  msg_size, buff, buff_size);

    if (decoded_len < 0) {
        return decoded_len;
    }

    if (decoded_len < 2) {
        return BETA_COM_ERR_MSG_TOO_SHORT; // Decoded message must be at least 2 bytes for CRC
    }

    // The CRC is calculated over the entire decoded message (payload + CRC bytes).
    // If the message is correct, the result will be 0.
    uint16_t crc = calculate_crc16(buff, decoded_len);
    if (crc != 0) {
        return BETA_COM_ERR_CRC_MISMATCH;
    }

    return decoded_len - 2; // Return size of the payload only (without CRC)
}

int32_t send_message(beta_com_handle_t *handle, const uint8_t *buff, size_t buff_size) {
    if (handle == NULL || buff == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    // Calculate CRC over the payload
    uint16_t crc = calculate_crc16(buff, buff_size);
    uint8_t crc_buff[2] = {(uint8_t) crc, (uint8_t) (crc >> 8)}; // Little-endian CRC

    // Create I/O vectors for payload and CRC to encode them together without copying
    beta_iovec_t crc_iovec = { .iov_base = crc_buff, .iov_len = 2 };
    beta_iovec_t data_iovec = { .iov_base = (uint8_t*)buff, .iov_len = buff_size };
    beta_iovec_t buffers[2] = {data_iovec, crc_iovec};

    // COBS encode the payload and CRC into the work buffer
    int32_t encoded_len = cobs_encode(buffers, 2, handle->tx_work_buff, handle->tx_wb_size);
    if (encoded_len < 0) return encoded_len;

    if (rb_free_size(&handle->tx_rb) < (size_t) encoded_len) return BETA_COM_ERR_RB_NOT_ENOUGH_SPACE;

    // Write the encoded message to the transmit ring buffer
    beta_com_err_t rb_write_err = rb_write_linear_block(&handle->tx_rb, handle->tx_work_buff, encoded_len);
    if (rb_write_err != 0) {
        return rb_write_err;
    }

    return encoded_len;
}