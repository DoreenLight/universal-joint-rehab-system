#include "main.h"

uint8_t u8Uart4TxBuffer[DF_UART4_REC_BUF_LEN];

UART_HandleTypeDef huart4;

static void Uart4GpioInit(void);
static void Uart4BlockingSend(uint8_t *u8Buf, uint16_t u16len);
volatile Uart4Diagnostics g_uart4_diag;

#if 1
#if (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");
__asm(".global __ARM_use_no_argv \n\t");
#else
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};
#endif

int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

FILE __stdout;

int fputc(int ch, FILE *f)
{
    uint8_t byte = (uint8_t)ch;
    (void)f;
    return Uart4SendFrame(&byte, 1U) == HAL_OK ? ch : EOF;
}
#endif

void Uart4Init(uint32_t u32baud)
{
    huart4.Instance = UART4;

    HAL_NVIC_DisableIRQ(UART4_IRQn);
    HAL_UART_DeInit(&huart4);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10 | GPIO_PIN_11);

    Uart4GpioInit();

    huart4.Init.BaudRate = u32baud;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart4) != HAL_OK)
    {
        Error_Handler();
    }
}

static void Uart4GpioInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART4;
    PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
    (void)HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    __HAL_RCC_UART4_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_UART4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Uart4DMASendFunction(uint8_t *u8Buf, uint16_t u16len)
{
    Uart4BlockingSend(u8Buf, u16len);
}

void Uart4DebugSendRxFrame(uint8_t *u8Buf, uint16_t u16len)
{
    Uart4BlockingSend(u8Buf, u16len);
}

static void Uart4BlockingSend(uint8_t *u8Buf, uint16_t u16len)
{
    (void)Uart4SendFrame(u8Buf, u16len);
}

HAL_StatusTypeDef Uart4SendFrame(const uint8_t *data, uint16_t length)
{
    HAL_StatusTypeDef status;
    if (data == NULL || length == 0U || length > DF_UART4_REC_BUF_LEN)
    {
        g_uart4_diag.invalid_requests++;
        return HAL_ERROR;
    }
    /* HAL timeouts require a running tick. Never block in an ISR or with
     * interrupts masked, including through the printf retarget above. */
    if (__get_IPSR() != 0U || __get_PRIMASK() != 0U)
    {
        g_uart4_diag.send_errors++;
        return HAL_BUSY;
    }
    /* Do not replay a partially transmitted frame or spin on UART flags.
     * Existing callers run in the main loop with the HAL tick enabled. */
    memcpy(u8Uart4TxBuffer, data, length);
    status = HAL_UART_Transmit(&huart4, u8Uart4TxBuffer, length, 100U);
    if (status == HAL_OK)
    {
        g_uart4_diag.sent_frames++;
        g_uart4_diag.sent_bytes += length;
    }
    else
    {
        g_uart4_diag.send_errors++;
        if (status == HAL_TIMEOUT) g_uart4_diag.timeouts++;
        if (status != HAL_BUSY) (void)HAL_UART_AbortTransmit(&huart4);
    }
    return status;
}
