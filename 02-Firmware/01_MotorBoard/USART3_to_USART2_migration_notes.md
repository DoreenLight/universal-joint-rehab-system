# USART3 to USART2 Migration Notes

Created: 2026-07-01

This file marks the places to check after changing the project from USART3 to USART2 in STM32CubeMX and regenerating code.

## CubeMX settings to change first

- Disable `USART3`.
- Enable `USART2` in asynchronous mode.
- Select the intended `USART2_TX` and `USART2_RX` pins.
- Enable `USART2 global interrupt`.
- If any code still uses `HAL_UART_Transmit_DMA()` on the migrated port, add `USART2_TX` DMA in CubeMX.
- RX DMA is not required for the current receive code, because it uses `HAL_UART_Receive_IT()`.

## Generated code that CubeMX should update

Do not hand-edit these before generation. After generation, only verify that they changed from USART3 to USART2.

- `Core/Src/usart.c`
  - `UART_HandleTypeDef huart3` should become or be replaced by `huart2`.
  - `DMA_HandleTypeDef hdma_usart3_tx` should become or be replaced by `hdma_usart2_tx` if USART2 TX DMA is enabled.
  - `MX_USART3_UART_Init()` should become or be replaced by `MX_USART2_UART_Init()`.
  - MSP init/deinit should use `USART2`, `__HAL_RCC_USART2_CLK_ENABLE()`, the new USART2 pins, and `GPIO_AF7_USART2`.
  - NVIC should use `USART2_IRQn`.
  - DMA link should use `__HAL_LINKDMA(uartHandle, hdmatx, hdma_usart2_tx)`.

- `Core/Inc/usart.h`
  - `extern UART_HandleTypeDef huart3;` should become or be replaced by `huart2`.
  - `void MX_USART3_UART_Init(void);` should become or be replaced by `MX_USART2_UART_Init`.

- `Core/Src/main.c`
  - Generated init call `MX_USART3_UART_Init();` should become or be replaced by `MX_USART2_UART_Init();`.

- `Core/Src/stm32f4xx_it.c`
  - `extern UART_HandleTypeDef huart3;` should become or be replaced by `huart2`.
  - `USART3_IRQHandler()` should become or be replaced by `USART2_IRQHandler()`.
  - `HAL_UART_IRQHandler(&huart3);` should become `HAL_UART_IRQHandler(&huart2);`.
  - If TX DMA is enabled, the DMA IRQ handler should call `HAL_DMA_IRQHandler(&hdma_usart2_tx);`.

- `Core/Inc/stm32f4xx_it.h`
  - `USART3_IRQHandler` declaration should become or be replaced by `USART2_IRQHandler`.
  - DMA IRQ declaration may change depending on the stream/channel CubeMX selects for `USART2_TX`.

- `Core/Src/dma.c`
  - DMA IRQ priority and enable lines should match the DMA stream CubeMX selects for `USART2_TX`.

- `Core/Inc/main.h`
  - `UART3_TX_Pin`, `UART3_RX_Pin`, and their GPIO ports should be replaced by the new labels/pins CubeMX generates for USART2, if labels are changed in CubeMX.

## User code to change manually after generation

These are inside `USER CODE` blocks, so CubeMX will preserve them but will not migrate them.

- `Core/Src/usart.c`
  - `g_usart3_rx_sta` and `g_usart3_rx_buffer` can be renamed to `g_usart2_rx_sta` and `g_usart2_rx_buffer`, or left as names only if you prefer. If renamed, update every reference and the header declarations.
  - In `USART*_Init 2` user block, change `HAL_UART_Receive_IT(&huart3, ...)` to `HAL_UART_Receive_IT(&huart2, ...)`.
  - In `HAL_UART_TxCpltCallback`, change `huart->Instance == USART3` to `USART2` if the DMA transmit busy flag belongs to the migrated port.
  - In `HAL_UART_RxCpltCallback`, change `huart->Instance == USART3` to `USART2`.
  - In `HAL_UART_RxCpltCallback`, change the receive buffer references if renamed.
  - At the end of `HAL_UART_RxCpltCallback`, restart receive with `HAL_UART_Receive_IT(&huart2, ...)`.
  - `uart3_SendDatas()` can be renamed to `uart2_SendDatas()`. If renamed, update the prototype and all calls.
  - Inside that send function, change `HAL_UART_Transmit_DMA(&huart3, ...)` to `HAL_UART_Transmit_DMA(&huart2, ...)`.

- `Core/Inc/usart.h`
  - Rename `g_usart3_rx_sta`, `g_usart3_rx_buffer`, and `uart3_SendDatas()` declarations if you rename them in `usart.c`.

- `Core/Src/main.c`
  - `uart3_SendDatas(Senddata_temp, data_No);` appears inside `#if 0`, so it is currently disabled. If the block is re-enabled later, update it to the new send function name if renamed.
  - `fputc()` currently transmits through `huart1`. This is unrelated to the USART3 to USART2 migration unless printf should also move to USART2.

## Important DMA note

The current project has `USART3_TX` DMA in the `.ioc` file. After moving to USART2:

- If using `HAL_UART_Transmit_DMA(&huart2, ...)`, CubeMX must configure `USART2_TX` DMA.
- Do not keep using the old `USART3_TX -> DMA1_Stream3` setup.
- The exact DMA stream for USART2 depends on CubeMX selection and pin/peripheral constraints, so verify the generated `hdma_usart2_tx` and IRQ handler names after generation.

## Final search after CubeMX generation

Run this search and inspect every remaining hit:

```text
USART3
huart3
MX_USART3
g_usart3
uart3_
hdma_usart3_tx
USART3_IRQn
DMA1_Stream3
UART3_
```

Expected result after a clean migration:

- No active `USART3`, `huart3`, `MX_USART3`, `hdma_usart3_tx`, or `USART3_IRQn` references should remain unless USART3 is intentionally still enabled.
- `DMA1_Stream3` should remain only if CubeMX also selected that exact stream for the new USART2 TX DMA, which is not something to assume.
- `huart1` references should remain if printf or telemetry is still intended to use USART1.
