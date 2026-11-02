#ifndef _USART4_H
#define _USART4_H

#include "main.h"

#define DF_UART4_BAUD_RARE   230400
#define DF_UART4_REC_BUF_LEN 1048

typedef struct {
    uint32_t sent_frames, sent_bytes, send_errors, timeouts, invalid_requests;
} Uart4Diagnostics;
extern volatile Uart4Diagnostics g_uart4_diag;
HAL_StatusTypeDef Uart4SendFrame(const uint8_t *data, uint16_t length);

void Uart4Init(uint32_t u32baud);
void Uart4DMASendFunction(uint8_t *u8Buf, uint16_t u16len);
void Uart4DebugSendRxFrame(uint8_t *u8Buf, uint16_t u16len);

#endif
