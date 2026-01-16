#include <assert.h>
#include <string.h>
#include "../beta_com.h"

// Simulation de mémoire
static uint8_t rx_storage[128];
static uint8_t tx_storage[128];
static uint8_t rx_work[128];
static uint8_t tx_work[128];

#include "test_msg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../beta_com.h"

// Helper to print buffer contents
static void print_bytes(const char* prefix, const uint8_t *bytes, size_t len) {
    printf("%s[%zu bytes]: ", prefix, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", bytes[i]);
    }
    printf("\n");
}

// Helper to print ring buffer state
static void print_ring_buffer_state(const char* name, const ring_buffer_t* rb) {
    size_t used = (rb->head - rb->tail + rb->max_size) % rb->max_size;
    printf("  %s state: head=%zu, tail=%zu, used=%zu, size=%zu\n", name, rb->head, rb->tail, used, rb->max_size);
}

void test_msg_loopback(void) {
    printf("--- TEST: MESSAGE SEND/RECEIVE LOOPBACK ---\n");

    // Memory simulation for the communication handle
    static uint8_t rx_storage[128];
    static uint8_t tx_storage[128];
    static uint8_t rx_work[128];
    static uint8_t tx_work[128];

    beta_com_handle_t h;
    beta_com_config_t conf = {
        .rx_rb_storage = rx_storage, .rx_rb_size = sizeof(rx_storage),
        .rx_work_buff = rx_work,     .rx_work_buff_size = sizeof(rx_work),
        .tx_rb_storage = tx_storage, .tx_rb_size = sizeof(tx_storage),
        .tx_work_buff = tx_work,     .tx_work_buff_size = sizeof(tx_work)
    };

    // 1. Initialization
    printf("Step 1: Initializing communication handle...\n");
    assert(beta_com_init(&h, &conf) == BETA_COM_SUCCESS);
    print_ring_buffer_state("TX Buffer", &h.tx_rb);
    print_ring_buffer_state("RX Buffer", &h.rx_rb);

    // 2. Send message
    const uint8_t msg_out[] = "Hello World!";
    printf("\nStep 2: Sending message...\n");
    print_bytes("  Payload to send ", msg_out, sizeof(msg_out));
    int32_t sent_len = send_message(&h, msg_out, sizeof(msg_out));
    assert(sent_len > 0);
    printf("  %d bytes written to TX work buffer and then pushed to TX ring buffer.\n", sent_len);
    print_ring_buffer_state("TX Buffer", &h.tx_rb);

    // 3. Hardware Loopback Simulation
    printf("\nStep 3: Simulating hardware loopback (TX -> RX)...\n");
    uint8_t byte;
    while(rb_pop(&h.tx_rb, &byte) == BETA_COM_SUCCESS) {
        assert(rb_push(&h.rx_rb, byte) == BETA_COM_SUCCESS);
    }
    printf("  Loopback complete. Data moved from TX to RX buffer.\n");
    print_ring_buffer_state("TX Buffer", &h.tx_rb);
    print_ring_buffer_state("RX Buffer", &h.rx_rb);

    // 4. Receive message
    uint8_t msg_in[64];
    printf("\nStep 4: Receiving message...\n");
    int32_t rcv_len = receive_message(&h, msg_in, sizeof(msg_in));
    assert(rcv_len > 0);
    print_bytes("  Received payload", (uint8_t*)msg_in, rcv_len);
    print_ring_buffer_state("RX Buffer", &h.rx_rb);

    // 5. Verification
    printf("\nStep 5: Verifying message integrity...\n");
    assert(rcv_len == sizeof(msg_out));
    assert(memcmp(msg_out, msg_in, rcv_len) == 0);
    printf("SUCCESS: Sent and received messages are identical.\n\n");
}