# MiddleBoard — 中间控制板固件（STM32G474）

位于触摸屏/上位机与电机板之间的中间控制板：负责串口链路转发、RS485 收发方向控制与外部设备（DR304）采集。

## 入口

| 用途 | 路径 |
| --- | --- |
| Keil MDK 工程 | `Projects/MDK-ARM/APP.uvprojx`（OutputName：`APP`） |

## 目录

| 目录 | 内容 |
| --- | --- |
| `User/` | `main.c`（分时任务主循环：5 / 10 / 25 / 50 / 100 ms 任务槽）、`Timer_pro.c`、HAL 配置 |
| `Driver/` | `Timer`、`USART`（UART1 / 3 / 4）、`SPI`（MX25L1606E SPI Flash）、`Watchdog`、`SysTick` |
| `Service/` | `CRC`、`Flash_Internal`（内部 Flash 读写）、`extern_device`（外部设备 GPIO 与 UART1 / 2 / 3 的 RS485 收发方向控制） |
| `Drivers/` | STM32G4 HAL、CMSIS，以及正点原子 `SYSTEM`（delay / sys / usart） |
| `Middlewares/`、`Projects/`、`Output/`、`tmp/` | 中间件占位说明、Keil 工程、编译输出、临时文件 |

## 链路（据 `Driver/USART/usart3.c`）

- **UART1**：115200；**UART3**：115200；**UART4**：230400（波特率宏见各 `Driver/USART/usart*.h`）。
- **UART3 → DR304**：以 Modbus 轮询读保持寄存器（地址与寄存器号由 `DF_DR304_*` 宏配置），支持参数保存与清零，带响应超时与轮询间隔控制，可选择把结果转发到 UART4。
- UART1 / 2 / 3 的 RS485 收发方向由 `Service/extern_device` 控制。

## 状态与注意

- 该工程无独立 git 仓库。
- `Output/` 与 `tmp/`（`pdf_pages`、探测脚本）为编译输出与临时文件，非源码。
- 基线来源：`User/main.c` 与 `Service/extern_device/extern_device_control.h` 头部标注为安诺科技提供的模板工程。
