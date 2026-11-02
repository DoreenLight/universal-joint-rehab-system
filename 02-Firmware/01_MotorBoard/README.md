# MotorBoard — 万向节电机板固件（STM32F407）

驱动万向节机构的电机控制板固件：接收上位机/中间板下发的帧，经 CAN1 控制 RS00 系列电机，并采集 AD7606 与 MPU6050 数据。

## 入口

| 用途 | 路径 |
| --- | --- |
| Keil MDK 工程 | `MDK-ARM/MotorBoard.uvprojx`（TargetName / OutputName：`MotorBoard`，产物 `MotorBoard.axf`、`MotorBoard.hex`） |
| STM32CubeMX 工程 | `MotorBoard.ioc` |
| 调试记录与协议文档 | `Document/`：关键调试的记录、上位机下发指令具体意义、Uart3 的通信协议、UART3 电机控制协议 |
| 串口迁移说明 | `USART3_to_USART2_migration_notes.md` |
| 电机手册 | `RS00使用说明书251210.pdf` |

## 目录

| 目录 | 内容 |
| --- | --- |
| `app/` | 应用层：`controller` / `motor_ctrl`（控制与状态）、`LADRC`（自抗扰控制）、`rs00_mit` / `rs00_motor`（RS00 电机 MIT 协议，走 CAN1）、`proto_transport` / `proto_app`（帧协议收发）、`cmd`（命令解析，含 AD7606 采样）、`shell`、`flash_store`（掉电参数保存） |
| `bsp/` | 板级驱动：`AD7606`、`mpu6050`、`i2c`、`delay` |
| `Core/` | CubeMX 生成的初始化与外设（CAN、USART、TIM、DMA、GPIO） |
| `Drivers/` | STM32F4 HAL 与 CMSIS |
| `MDK-ARM/` | Keil 工程、启动文件、RTE 与编译输出（`MDK-ARM/MotorBoard/`） |
| `UART_Reader/` | 早期随工程携带的上位机副本，与 `03-HostApp/02_SerialReader` 同源但内容有差异 |
| `scripts/`、`fix_keil.py`、`.vscode/` | 辅助脚本与编辑器配置 |

## 依赖

- Keil MDK-ARM（uVision 5）+ STM32F4xx 器件支持包；重新生成代码需 STM32CubeMX
- 目标芯片：STM32F407VETx；调试器：J-Link（`MDK-ARM/JLinkSettings.ini`）

## 状态与注意

- 该工程有独立 git 仓库，工作区存在未提交改动（Keil/CubeMX 生成文件）；提交前请先核对差异。
- `MDK-ARM/MotorBoard/` 内为历史编译产物（`.axf` / `.hex` / `.map` / `.o` 等），本次整理只做了改名，未重新编译；下次在 Keil 编译会生成同名新产物。
