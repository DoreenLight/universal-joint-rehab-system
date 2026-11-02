/**
  ******************************************************************************
  * @file    proto_app.c
  * @brief   UART3 Application Layer implementation
  ******************************************************************************
  */

#include "proto_app.h"
#include "proto_transport.h"
#include "flash_store.h"
#include <stdio.h>
#include <string.h>

/* --- Static callbacks --- */
static proto_app_float_cb_t g_app_float_cb = NULL;
static proto_app_io_cb_t    g_app_io_cb = NULL;

/* --- Helper: pack float to 4 bytes little-endian --- */
static void proto_pack_float(uint8_t *dest, float value)
{
    uint32_t raw;
    (void)memcpy(&raw, &value, 4U);
    dest[0] = (uint8_t)(raw & 0xFFU);
    dest[1] = (uint8_t)((raw >> 8) & 0xFFU);
    dest[2] = (uint8_t)((raw >> 16) & 0xFFU);
    dest[3] = (uint8_t)((raw >> 24) & 0xFFU);
}

/* --- Helper: unpack 4 bytes little-endian to float --- */
static float proto_unpack_float(const uint8_t *data)
{
    uint32_t raw;
    float    value;
    raw  = (uint32_t)data[0];
    raw |= (uint32_t)data[1] << 8;
    raw |= (uint32_t)data[2] << 16;
    raw |= (uint32_t)data[3] << 24;
    (void)memcpy(&value, &raw, 4U);
    return value;
}

/* --- Send a single float (convenience) --- */
uint8_t proto_app_send_float(uint8_t float_id, float value)
{
    return proto_app_send_telemetry(1U, &float_id, &value);
}

/* --- Send multiple floats in one frame --- */
uint8_t proto_app_send_telemetry(uint8_t count,
                                 const uint8_t *float_ids,
                                 const float *values)
{
    uint8_t  payload[1U + PROTO_TELEM_MAX_FLOATS * 5U]; /* 1 + 12*5 = 61 */
    uint8_t  i;

    if (count > PROTO_TELEM_MAX_FLOATS)
    {
        count = PROTO_TELEM_MAX_FLOATS;
    }

    payload[0] = count;
    for (i = 0U; i < count; i++)
    {
        uint8_t base = (uint8_t)(1U + i * 5U);
        payload[base] = float_ids[i];
        proto_pack_float(&payload[base + 1U], values[i]);
    }

    return proto_transport_send(PROTO_CMD_TELEMETRY,
                                payload,
                                (uint8_t)(1U + count * 5U));
}

/* --- Send an IO signal --- */
uint8_t proto_app_send_io(uint8_t io_id, uint8_t state)
{
    uint8_t payload[2];
    payload[0] = io_id;
    payload[1] = state;
    return proto_transport_send(PROTO_CMD_IO_REPORT, payload, 2U);
}

/* --- Dispatch by count helper --- */
static void proto_app_dispatch_floats(const uint8_t *payload, uint8_t len)
{
    uint8_t count = payload[0];
    uint8_t idx   = 1U;
    uint8_t i;

    printf("[APP] Floats cnt=%u\r\n", (unsigned int)count);

    for (i = 0U; i < count && (idx + 5U) <= len; i++)
    {
        uint8_t float_id = payload[idx];
        float   value    = proto_unpack_float(&payload[idx + 1U]);

        printf("[APP]   id=0x%02X val=%.4f\r\n",
               (unsigned int)float_id, (double)value);

        if (g_app_float_cb != NULL)
        {
            g_app_float_cb(float_id, value);
        }

        idx += 5U;
    }
}

/* --- Frame dispatch from transport layer --- */
void proto_app_handle_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    switch (cmd)
    {
    case PROTO_CMD_IO_WRITE:
        if (len >= 2U)
        {
            uint8_t io_id  = payload[0];
            uint8_t io_val = payload[1];
            printf("[APP] IO write id=0x%02X val=%u\r\n",
                   (unsigned int)io_id, (unsigned int)io_val);
            if (g_app_io_cb != NULL)
            {
                g_app_io_cb(io_id, io_val);
            }
        }
        break;

    case PROTO_CMD_FLOAT_WRITE:
        if (len >= 5U)
        {
            uint8_t float_id = payload[0];
            float   value    = proto_unpack_float(&payload[1]);
            printf("[APP] Float write id=0x%02X val=%.4f\r\n",
                   (unsigned int)float_id, (double)value);
            if (g_app_float_cb != NULL)
            {
                g_app_float_cb(float_id, value);
            }
        }
        break;

    case PROTO_CMD_BATCH_WRITE:
        if (len >= 1U)
        {
            proto_app_dispatch_floats(payload, len);
        }
        break;

    case PROTO_CMD_TELEMETRY:
        if (len >= 1U)
        {
            proto_app_dispatch_floats(payload, len);
        }
        break;

    case PROTO_CMD_IO_REPORT:
        printf("[APP] IO report (ignored)\r\n");
        break;

    case PROTO_CMD_ACK:
        if (len >= 1U)
        {
            printf("[APP] ACK for cmd=0x%02X\r\n", (unsigned int)payload[0]);
        }
        break;

    case PROTO_CMD_PING:
        printf("[APP] Ping\r\n");
        break;

    case PROTO_CMD_FLASH_SAVE:
        printf("[APP] Flash save command\r\n");
        flash_store_save();
        break;

    default:
        printf("[APP] Unknown cmd=0x%02X len=%u\r\n",
               (unsigned int)cmd, (unsigned int)len);
        break;
    }
}

/* --- Register callbacks --- */
void proto_app_set_float_callback(proto_app_float_cb_t cb)
{
    g_app_float_cb = cb;
}

void proto_app_set_io_callback(proto_app_io_cb_t cb)
{
    g_app_io_cb = cb;
}

/* --- Init --- */
void proto_app_init(void)
{
    proto_transport_set_rx_callback(proto_app_handle_frame);
    printf("PROTO: app layer ready\r\n");
}
