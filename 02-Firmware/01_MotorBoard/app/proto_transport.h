/**
  ******************************************************************************
  * @file    proto_transport.h
  * @brief   UART3 Transport Layer: frame sync, 8-bit sum checksum
  ******************************************************************************
  */

#ifndef PROTO_TRANSPORT_H
#define PROTO_TRANSPORT_H

#include <stdint.h>

/* --- Frame format ---
   [SYNC0 0x6C] [SYNC1 0x67] [LEN] [CMD] [PAYLOAD 0..250B] [SUM 1B] [END 0xED]

   LEN = payload byte count (0 = empty payload, valid)
   SUM  = (LEN + CMD + sum of all PAYLOAD bytes) & 0xFF
   END  = 0xED  (frame tail, prevents false sync from float data)
   Minimum frame: 6 bytes  (sync×2 + len=0 + cmd + sum + end)
   Maximum frame: 256 bytes (sync×2 + len=250 + cmd + sum + end)
--- */

#define PROTO_SYNC0          0x6Cu   /* 'l' */
#define PROTO_SYNC1          0x67u   /* 'g' */
#define PROTO_END_MARKER      0xEDu  /* frame tail */
#define PROTO_MAX_PAYLOAD    250u
#define PROTO_MAX_FRAME      (PROTO_MAX_PAYLOAD + 6u)
#define PROTO_HEADER_SIZE    4u      /* SYNC0 + SYNC1 + LEN + CMD */
#define PROTO_FOOTER_SIZE    2u      /* 8-bit sum + END marker */

/* --- Error codes --- */
#define PROTO_ERR_NONE       0u
#define PROTO_ERR_CHECKSUM   1u
#define PROTO_ERR_LENGTH     2u
#define PROTO_ERR_SYNC       3u
#define PROTO_ERR_BUSY       4u

/* --- Transport-level commands --- */
#define PROTO_CMD_PING       0x84u   /* Host->MCU: ping, transport auto-ACKs */
#define PROTO_CMD_ACK        0x83u   /* Bidirectional: ACK [ack_cmd] */

/* --- Callback types --- */
typedef void (*proto_rx_callback_t)(uint8_t cmd,
                                    const uint8_t *payload,
                                    uint8_t len);

typedef void (*proto_error_callback_t)(uint8_t error_code);

/* --- Public API --- */
void proto_transport_init(void);
uint8_t proto_transport_send(uint8_t cmd,
                             const uint8_t *payload,
                             uint8_t len);
void proto_transport_rx_process(const uint8_t *data, uint16_t len);
void proto_transport_poll(void);   /* Call in main loop to drain RX queue */
void proto_transport_set_rx_callback(proto_rx_callback_t cb);
void proto_transport_set_error_callback(proto_error_callback_t cb);

#endif /* PROTO_TRANSPORT_H */
