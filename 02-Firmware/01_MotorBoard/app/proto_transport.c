/**
  ******************************************************************************
  * @file    proto_transport.c
  * @brief   UART3 Transport Layer: 8-bit sum checksum, frame sync, DMA send
  ******************************************************************************
  */

#include "proto_transport.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* --- Parser states --- */
typedef enum {
    PSTATE_IDLE,
    PSTATE_SYNC1,
    PSTATE_HEADER,
    PSTATE_DATA
} ProtoRxState;

/* --- Static variables --- */
static ProtoRxState          g_proto_state = PSTATE_IDLE;
static uint8_t               g_proto_rx_buf[PROTO_MAX_FRAME];
static uint8_t               g_proto_rx_idx;
static uint8_t               g_proto_rx_len;
static uint8_t               g_proto_rx_remaining;
static proto_rx_callback_t   g_proto_rx_cb = NULL;
static proto_error_callback_t g_proto_err_cb = NULL;
static uint32_t              g_proto_rx_frame_count = 0U;
static uint32_t              g_proto_rx_chk_err_count = 0U;
static uint32_t              g_proto_rx_queue_overflow = 0U;

/* --- RX ring queue (ISR push, main loop pop) --- */
#define PROTO_RX_QUEUE_SIZE  8u

typedef struct {
    uint8_t  cmd;
    uint8_t  len;
    uint8_t  data[PROTO_MAX_PAYLOAD];
} proto_rx_entry_t;

static proto_rx_entry_t g_rx_queue[PROTO_RX_QUEUE_SIZE];
static volatile uint8_t g_rx_queue_head = 0u;  /* ISR writes */
static uint8_t          g_rx_queue_tail = 0u;  /* main reads */

static uint8_t proto_queue_push(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t next = (uint8_t)((g_rx_queue_head + 1u) % PROTO_RX_QUEUE_SIZE);
    if (next == g_rx_queue_tail)
    {
        return 0u;  /* full */
    }
    g_rx_queue[g_rx_queue_head].cmd = cmd;
    g_rx_queue[g_rx_queue_head].len = len;
    if (len > 0u)
    {
        (void)memcpy(g_rx_queue[g_rx_queue_head].data, payload, len);
    }
    g_rx_queue_head = next;
    return 1u;
}

static uint8_t proto_queue_pop(proto_rx_entry_t *entry)
{
    if (g_rx_queue_head == g_rx_queue_tail)
    {
        return 0u;  /* empty */
    }
    entry->cmd = g_rx_queue[g_rx_queue_tail].cmd;
    entry->len = g_rx_queue[g_rx_queue_tail].len;
    if (entry->len > 0u)
    {
        (void)memcpy(entry->data, g_rx_queue[g_rx_queue_tail].data, entry->len);
    }
    g_rx_queue_tail = (uint8_t)((g_rx_queue_tail + 1u) % PROTO_RX_QUEUE_SIZE);
    return 1u;
}

/* --- 8-bit sum checksum --- */
static uint8_t proto_checksum(const uint8_t *data, uint16_t len)
{
    uint8_t sum = 0U;
    while (len-- != 0U)
    {
        sum += *data++;
    }
    return sum;
}

/* --- Build and send a frame --- */
static void proto_send_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[PROTO_MAX_FRAME];
    uint8_t chk;

    if (len > PROTO_MAX_PAYLOAD)
    {
        return;
    }

    frame[0] = PROTO_SYNC0;
    frame[1] = PROTO_SYNC1;
    frame[2] = len;
    frame[3] = cmd;

    if (len > 0U)
    {
        (void)memcpy(&frame[4], payload, len);
    }

    /* Checksum over [LEN, CMD, PAYLOAD] = bytes 2..(3+len) */
    chk = proto_checksum(&frame[2], (uint16_t)(len + 2U));
    frame[4 + len] = chk;
    frame[5 + len] = PROTO_END_MARKER;

    (void)UART3_SendDMA(frame, (uint16_t)(PROTO_HEADER_SIZE + len + PROTO_FOOTER_SIZE));
}

/* --- Public API --- */

void proto_transport_init(void)
{
    g_proto_state          = PSTATE_IDLE;
    g_proto_rx_idx         = 0U;
    g_proto_rx_frame_count = 0U;
    g_proto_rx_chk_err_count = 0U;

    printf("PROTO: transport ready (8-bit sum)\r\n");
}

uint8_t proto_transport_send(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    if (g_usart3_dma_tx_busy)
    {
        return PROTO_ERR_BUSY;
    }

    proto_send_frame(cmd, payload, len);
    return PROTO_ERR_NONE;
}

void proto_transport_rx_process(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t  byte;

    for (i = 0U; i < len; i++)
    {
        byte = data[i];

        switch (g_proto_state)
        {
        case PSTATE_IDLE:
            if (byte == PROTO_SYNC0)
            {
                g_proto_state = PSTATE_SYNC1;
            }
            break;

        case PSTATE_SYNC1:
            if (byte == PROTO_SYNC1)
            {
                g_proto_state  = PSTATE_HEADER;
                g_proto_rx_idx = 0U;
            }
            else if (byte != PROTO_SYNC0)
            {
                g_proto_state = PSTATE_IDLE;
            }
            /* byte == PROTO_SYNC0: back-to-back sync, stay in SYNC1 */
            break;

        case PSTATE_HEADER:
            /* Collect LEN and CMD (2 bytes) */
            g_proto_rx_buf[g_proto_rx_idx++] = byte;

            if (g_proto_rx_idx >= 2U)
            {
                g_proto_rx_len = g_proto_rx_buf[0];

                if (g_proto_rx_len > PROTO_MAX_PAYLOAD)
                {
                    if (g_proto_err_cb != NULL)
                    {
                        g_proto_err_cb(PROTO_ERR_LENGTH);
                    }
                    g_proto_state = (byte == PROTO_SYNC0) ? PSTATE_SYNC1 : PSTATE_IDLE;
                }
                else
                {
                    g_proto_state       = PSTATE_DATA;
                    g_proto_rx_remaining = (uint8_t)(g_proto_rx_len + PROTO_FOOTER_SIZE);
                }
            }
            break;

        case PSTATE_DATA:
            /* Collect PAYLOAD + CHECKSUM bytes */
            g_proto_rx_buf[g_proto_rx_idx++] = byte;
            g_proto_rx_remaining--;

            if (g_proto_rx_remaining == 0U)
            {
                uint8_t payload_len = g_proto_rx_len;
                uint8_t rx_cmd      = g_proto_rx_buf[1];
                uint8_t chk_calc, chk_rx;

                /* Verify 8-bit sum checksum over [LEN, CMD, PAYLOAD] */
                chk_calc = proto_checksum(g_proto_rx_buf, (uint16_t)(payload_len + 2U));
                chk_rx   = g_proto_rx_buf[2U + payload_len];

                uint8_t end_marker = g_proto_rx_buf[3U + payload_len];

                if (chk_calc == chk_rx && end_marker == PROTO_END_MARKER)
                {
                    g_proto_rx_frame_count++;

                    /* Push to queue (ISR-safe); callback fires from main loop */
                    if (!proto_queue_push(rx_cmd, &g_proto_rx_buf[2U], payload_len))
                    {
                        g_proto_rx_queue_overflow++;
                    }

                    /* Auto-ACK for PING (lightweight, safe in ISR) */
                    if (rx_cmd == PROTO_CMD_PING)
                    {
                        proto_send_frame(PROTO_CMD_ACK, &rx_cmd, 1U);
                    }
                }
                else
                {
                    g_proto_rx_chk_err_count++;
                    if (g_proto_err_cb != NULL)
                    {
                        g_proto_err_cb(PROTO_ERR_CHECKSUM);
                    }
                }

                g_proto_state = (byte == PROTO_SYNC0) ? PSTATE_SYNC1 : PSTATE_IDLE;
            }
            break;

        default:
            g_proto_state = PSTATE_IDLE;
            break;
        }
    }
}

/* --- Poll: drain RX queue in main loop context --- */
void proto_transport_poll(void)
{
    proto_rx_entry_t entry;

    while (proto_queue_pop(&entry))
    {
        if (g_proto_rx_cb != NULL)
        {
            g_proto_rx_cb(entry.cmd, entry.data, entry.len);
        }
    }
}

void proto_transport_set_rx_callback(proto_rx_callback_t cb)
{
    g_proto_rx_cb = cb;
}

void proto_transport_set_error_callback(proto_error_callback_t cb)
{
    g_proto_err_cb = cb;
}
