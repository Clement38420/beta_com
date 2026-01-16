#include "beta_com.h"

#include <stdlib.h>
#include <string.h>

beta_com_err_t beta_com_init(beta_com_handle_t *handle, const beta_com_config_t *config) {
    if (handle == NULL || config == NULL) return BETA_COM_ERR_INVALID_ARGS;

    if (config->use_dynamic_alloc == 1) {
        handle->is_dynamic = 1;

        int err_code = 0;
        handle->tx_work_buff = malloc(config->tx_work_buff_size);
        err_code |= (handle->tx_work_buff == NULL);
        handle->rx_work_buff = malloc(config->rx_work_buff_size);
        err_code |= (handle->rx_work_buff == NULL);
        handle->tx_rb.buffer = malloc(config->tx_rb_size);
        err_code |= (handle->tx_rb.buffer == NULL);
        handle->rx_rb.buffer = malloc(config->rx_rb_size);
        err_code |= (handle->rx_rb.buffer == NULL);

        if (err_code != 0) {
            free(handle->tx_work_buff);
            free(handle->rx_work_buff);
            free(handle->tx_rb.buffer);
            free(handle->rx_rb.buffer);
            return BETA_COM_ERR_OUT_OF_MEMORY;
        }

        handle->tx_wb_size = config->tx_work_buff_size;
        handle->rx_wb_size = config->rx_work_buff_size;

        handle->tx_rb = (ring_buffer_t){
            .buffer = handle->tx_rb.buffer,
            .head = 0,
            .tail = 0,
            .max_size = config->tx_rb_size
        };
        handle->rx_rb = (ring_buffer_t){
            .buffer = handle->rx_rb.buffer,
            .head = 0,
            .tail = 0,
            .max_size = config->rx_rb_size
        };

        return BETA_COM_SUCCESS;
    }

    if (config->rx_rb_storage == NULL) return BETA_COM_ERR_INVALID_ARGS;
    handle->rx_rb = (ring_buffer_t){
        .buffer = config->rx_rb_storage,
        .head = 0,
        .tail = 0,
        .max_size = config->rx_rb_size
    };

    if (config->rx_work_buff == NULL) return BETA_COM_ERR_INVALID_ARGS;
    handle->rx_work_buff = config->rx_work_buff;
    handle->rx_wb_size = config->rx_work_buff_size;

    if (config->tx_rb_storage == NULL) return BETA_COM_ERR_INVALID_ARGS;
    handle->tx_rb = (ring_buffer_t){
        .buffer = config->tx_rb_storage,
        .head = 0,
        .tail = 0,
        .max_size = config->tx_rb_size
    };

    if (config->tx_work_buff == NULL) return BETA_COM_ERR_INVALID_ARGS;
    handle->tx_work_buff = config->tx_work_buff;
    handle->tx_wb_size = config->tx_work_buff_size;
    return BETA_COM_SUCCESS;
}

beta_com_err_t beta_com_init_easy(beta_com_handle_t *handle, size_t max_payload_size) {
    if (max_payload_size == 0) return BETA_COM_ERR_INVALID_ARGS;

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

    if (handle->is_dynamic) {
        free(handle->tx_work_buff);
        free(handle->rx_work_buff);
        free(handle->tx_rb.buffer);
        free(handle->rx_rb.buffer);
    }

    memset(handle, 0, sizeof(beta_com_handle_t));
}

int32_t cobs_encode(const beta_iovec_t *buffers, size_t buffers_count, uint8_t *output, size_t max_out_len) {
    if (buffers == NULL || output == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    uint8_t *p_out = output;
    uint8_t *p_out_end = output + max_out_len;
    if (max_out_len < 2) return BETA_COM_ERR_BUFFER_TOO_SMALL; // Need at least space for code byte and trailing zero

    uint8_t *p_code = p_out++; // Reserve space for the first code byte
    uint8_t code = 1;

    for (size_t i = 0; i < buffers_count; i++) {
        const uint8_t *p_in = buffers[i].iov_base;
        const uint8_t *p_in_end = buffers[i].iov_base + buffers[i].iov_len;

        while (p_in < p_in_end) {
            if (*p_in == 0) {
                *p_code = code;
                if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                p_code = p_out++;
                code = 1;
            } else {
                if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                *p_out++ = *p_in;
                code++;

                if (code == 0xFF) { // Maximum code value reached
                    *p_code = code;
                    if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
                    p_code = p_out++;
                    code = 1;
                }
            }
            p_in++;
        }
    }

    // Frame ending
    *p_code = code;

    if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
    *p_out++ = 0x00; // Add the trailing zero byte

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

        if (code == 0) break;

        if (p_in + code - 1 > p_in_end) return BETA_COM_ERR_INVALID_DATA;

        for (uint8_t i = 1; i < code; i++) {
            if (p_out >= p_out_end) return BETA_COM_ERR_BUFFER_TOO_SMALL;
            *p_out++ = *p_in++;
        }

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

    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i<length; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
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

beta_com_err_t rb_push(ring_buffer_t *rb, uint8_t data) {
    size_t next_h = (rb->head + 1) % rb->max_size;

    if (next_h == rb->tail) return BETA_COM_ERR_RB_FULL;

    rb->buffer[rb->head] = data;
    rb->head = next_h;
    return BETA_COM_SUCCESS;
}

beta_com_err_t rb_pop(ring_buffer_t *rb, uint8_t *data) {
    if (rb->tail == rb->head) return BETA_COM_ERR_RB_EMPTY;

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->max_size;
    return BETA_COM_SUCCESS;
}

size_t rb_available_size(ring_buffer_t rb) {
    return rb.tail <= rb.head ? rb.tail + rb.max_size - rb.head - 1 : rb.tail - rb.head - 1;
}

uint8_t* rbchr(const ring_buffer_t *rb, uint8_t byte) {
    for (size_t i = rb->tail; i != rb->head; ) {
        if (rb->buffer[i] == byte) return &(rb->buffer[i]);
        i = (i + 1) % rb->max_size;
    }
    return NULL;
}

int32_t receive_message(beta_com_handle_t *handle, uint8_t *buff, size_t buff_size) {
    uint8_t *p_end = rbchr(&(handle->rx_rb), 0x00);
    if (p_end == NULL) return BETA_COM_ERR_NO_MESSAGE_FOUND;

    size_t end = p_end - handle->rx_rb.buffer;
    size_t msg_size = end < handle->rx_rb.tail ? end + handle->rx_rb.max_size - handle->rx_rb.tail : end - handle->rx_rb.tail;
    if (msg_size > buff_size) {
        handle->rx_rb.tail = (end + 1) % handle->rx_rb.max_size;
        return BETA_COM_ERR_BUFFER_TOO_SMALL;
    }

    uint8_t *p_wb = handle->rx_work_buff;
    while (handle->rx_rb.tail != end) {
        beta_com_err_t pop_err = rb_pop(&(handle->rx_rb), p_wb);
        if (pop_err != BETA_COM_SUCCESS) return pop_err;

        p_wb++;
        if ((size_t)(p_wb - handle->rx_work_buff) > handle->rx_wb_size) {
            return BETA_COM_ERR_BUFFER_TOO_SMALL;
        }
    }
    uint8_t dummy;
    rb_pop(&(handle->rx_rb), &dummy);

    int32_t decoded_len = cobs_decode(handle->rx_work_buff,  msg_size, buff, buff_size);

    if (decoded_len < 0) {
        return decoded_len;
    }

    if (decoded_len < 2) {
        return BETA_COM_ERR_MSG_TOO_SHORT;
    }

    uint16_t crc = calculate_crc16(buff, decoded_len); // The result is 0 if the correct CRC take up the last 2 bytes in little-endian
    if (crc != 0) {
        return BETA_COM_ERR_CRC_MISMATCH;
    }

    return decoded_len - 2; // Return size without CRC
}

int32_t send_message(beta_com_handle_t *handle, const uint8_t *buff, size_t buff_size) {
    if (handle == NULL || buff == NULL) {
        return BETA_COM_ERR_INVALID_ARGS;
    }

    uint16_t crc = calculate_crc16(buff, buff_size);
    uint8_t crc_buff[2] = {(uint8_t) crc, (uint8_t) (crc >> 8)};
    beta_iovec_t crc_iovec = (beta_iovec_t) {
        .iov_base = crc_buff,
        .iov_len = 2
    };

    beta_iovec_t data_iovec = (beta_iovec_t) {
        .iov_base = (uint8_t*)buff,
        .iov_len = buff_size
    };

    beta_iovec_t buffers[2] = {data_iovec, crc_iovec};

    int32_t encoded_len = cobs_encode(buffers, 2, handle->tx_work_buff, handle->tx_wb_size);
    if (encoded_len < 0) return encoded_len;

    if (rb_available_size(handle->tx_rb) < encoded_len) return BETA_COM_ERR_RB_NOT_ENOUGH_SPACE;

    uint8_t *p_wb = handle->tx_work_buff;
    uint8_t *p_wb_end = handle->tx_work_buff + encoded_len;
    while (p_wb != p_wb_end) {
        beta_com_err_t push_err = rb_push(&(handle->tx_rb), *p_wb);
        if (push_err != BETA_COM_SUCCESS) return push_err;

        p_wb++;
    }

    return encoded_len;
}