# 万向节康复系统

面向腕关节康复训练的万向节（十字万向节机构）控制系统，由 STM32 固件、ESP32-S3 触摸屏固件与 Qt 6 上位机组成，配套 USBCAN 分析仪的调试资料。

> 本目录为工程归档：固件源码、上位机源码、硬件资料与历史快照。每个工程目录内另有 `README.md` 说明用途、入口与依赖；`02-Firmware/01_MotorBoard` 与 `03-HostApp/01_SerialConsole` 保留各自的独立 git 仓库，根目录不设仓库。

## 目录结构

| 目录 | 内容 |
| --- | --- |
| [`01-CANAnalyzer/`](01-CANAnalyzer/) | CANalyst-II / USBCAN 分析仪资料：产品说明书与规格书、二次开发库（`ControlCAN`：VC / VB / Delphi / LabVIEW）、USB 驱动、原厂与周立功调试工具 |
| [`02-Firmware/`](02-Firmware/) | 嵌入式固件 |
| └ [`01_MotorBoard/`](02-Firmware/01_MotorBoard/) | **电机板 STM32F407**（CubeMX + Keil MDK）：LADRC 控制、RS00 电机 MIT 协议、CAN/USART 通信、AD7606 与 MPU6050 采集、SPI Flash；含 `Document/` 调试记录与 USART3→USART2 迁移说明 |
| └ [`02_MiddleBoard/`](02-Firmware/02_MiddleBoard/) | **中间控制板 STM32G474**（Keil MDK）：UART1/3/4 转发、DR304 的 Modbus 轮询、内部 Flash 与 CRC 服务、MX25L1606E SPI Flash |
| └ [`03_TouchPanel/`](02-Firmware/03_TouchPanel/) | **触摸屏 ESP32-S3**（微雪 ESP32-S3-Touch-LCD-4.3，Arduino + LVGL 8 + SquareLine）：界面逻辑、BLE 服务与 RS485 电机控制协议 |
| [`03-HostApp/`](03-HostApp/) | Qt 6 上位机（CMake 工程） |
| └ [`01_SerialConsole/`](03-HostApp/01_SerialConsole/) | 串口控制台「智能康腕系统」：电机指令下发、帧校验与遥测记录（`telemetry_*.csv`） |
| └ [`02_SerialReader/`](03-HostApp/02_SerialReader/) | 串口读取器：电机控制帧协议（`MotorUartProtocol`）解析与调试，含 `通信协议.xlsx` |
| └ [`03_BleConsole/`](03-HostApp/03_BleConsole/) | 蓝牙上位机：掌屈/背伸康复训练流程、便携版发布脚本（`build_*_release.cmd`）、开发计划文档 |
| [`09-Archive/`](09-Archive/) | 历史压缩包快照（内容未改动，仅供追溯） |

## 现有链路（据 `RS485_PROTOCOL.md` 与蓝牙上位机开发计划）

```text
Qt 蓝牙上位机 ──BLE──> ESP32-S3 触摸屏 ──RS485──> 中间板 STM32G474 ──UART──> 电机板 STM32F407 ──> RS00 电机
Qt 串口上位机 ──UART─────────────────────────────> 中间板 STM32G474
```

## 打开方式

| 工程 | 入口 |
| --- | --- |
| 电机板 | Keil MDK：`02-Firmware/01_MotorBoard/MDK-ARM/MotorBoard.uvprojx`；CubeMX：`02-Firmware/01_MotorBoard/MotorBoard.ioc` |
| 中间板 | Keil MDK：`02-Firmware/02_MiddleBoard/Projects/MDK-ARM/APP.uvprojx` |
| 触摸屏 | Arduino IDE：`02-Firmware/03_TouchPanel/03_TouchPanel.ino`（目录名与 `.ino` 必须同名，改名时需同步） |
| 上位机 | Qt Creator 或 CMake 打开各自的 `CMakeLists.txt`（target：`Uart_controller` / `UART` / `BleConsole`） |

## 本轮整理对照表

| 整理前 | 整理后 |
| --- | --- |
| `CAN分析仪资料20250618` | `01-CANAnalyzer` |
| `Motor_wanguanjie_v3_20260325_lg` | `02-Firmware/01_MotorBoard` |
| `MIDDLE` | `02-Firmware/02_MiddleBoard` |
| `sketch_jun15b` | `02-Firmware/03_TouchPanel` |
| `Uart_controller` | `03-HostApp/01_SerialConsole` |
| `UART_Reader` | `03-HostApp/02_SerialReader` |
| `wanguanjieBLE` | `03-HostApp/03_BleConsole` |
| `Motor_wanguanjie_v3_20260325_lg.rar` | `09-Archive/MotorBoard_v3_20260325_lg.rar` |
| `Uart_controller.rar` / `.zip` | `09-Archive/SerialConsole.rar` / `SerialConsole.zip` |
| 根目录 `minda-beigong-ziyouduwankang (2)` | `universal-joint-rehab-system` |
| Keil 工程 `Motor_wanguanjie.uvprojx`（TargetName / OutputName / 输出目录 / RTE / `.uvoptx` / `.uvguix` / `.ioc`） | `MotorBoard.uvprojx` 等，产物 `MotorBoard.axf`、`MotorBoard.hex` |
| Qt 蓝牙上位机 target `wanguanjieBLE`（含 exe 与 `build/` 内生成物） | target `BleConsole`，产物 `BleConsole.exe` |
| 协议类 `DiaoCangUartProtocol` 与文件 `DiaoCang_Uart_protocol.cpp` / `diaocang_uart_protocol.h`（串口读取器及其在电机板的副本） | 类 `MotorUartProtocol`、文件 `MotorUartProtocol.cpp` / `MotorUartProtocol.h` |
| 复选框控件 `chk_DiaoCang_Jieshou_Protocol` 与槽 `onDiaoCangCheckboxStateChanged` | `chk_receiveProtocol` 与 `onReceiveProtocolCheckboxStateChanged` |

改名的边界：只改目录名、工程/产物命名与指向它们的引用，不改源码逻辑。因 Arduino 要求目录名与 `.ino` 同名，`03_TouchPanel.ino` 由 `sketch_jun15b.ino` 改名而来，`RS485_PROTOCOL.md` 中的引用已同步更新。

**构建产物说明**：`MDK-ARM/MotorBoard/`、`MDK-ARM/DebugConfig/` 与 `03_BleConsole/build/` 内的产物（`.axf` / `.hex` / `.map` / `.exe` 等）本次只做了改名，**未重新编译**；下次在 Windows 上重新编译才会生成内容与名称都一致的新产物，若需彻底清干净，可删除这些构建目录后重建。`MDK-ARM/RTE/_MotorBoard/RTE_Components.h` 与 `MDK-ARM/*.log`、`startup_*.lst` 等日志/生成文件内仍留有旧工程名字符串，Keil 下次编译会重写。

## 第三方资料

`01-CANAnalyzer/` 的说明书、规格书与 `ControlCAN` 二次开发库，以及电机板的 `RS00使用说明书251210.pdf`，均为第三方资料，著作权归各原厂所有，仅供内部调试参考。
