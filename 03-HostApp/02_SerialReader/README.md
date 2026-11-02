# SerialReader — 串口读取器（Qt 6）

按 `MotorUartProtocol` 电机控制帧协议解析串口帧的上位机工具，用于协议核对与联调调试。

## 入口

| 用途 | 路径 |
| --- | --- |
| CMake 工程 | `UART/CMakeLists.txt`（target：`UART`，产物 `UART.exe`） |
| 协议实现 | `UART/MotorUartProtocol.cpp`、`UART/MotorUartProtocol.h`（类 `MotorUartProtocol`，原 `DiaoCangUartProtocol`） |
| 串口线程与帧处理 | `UART/SerialWorker.cpp`、`UART/protocolhandler.cpp` |
| 界面 | `UART/mainwindow.cpp` / `mainwindow.ui` / `mainwindow.h` |
| 协议表 | `通信协议.xlsx` |

## 依赖

- Qt 6.5+：`Core`、`Widgets`、`SerialPort`

## 状态与注意

- 该工程无独立 git 仓库；`UART/build/` 为编译输出。
- 本实现与 ESP32 触摸屏固件下发的帧格式一致，对应说明见 `02-Firmware/03_TouchPanel/RS485_PROTOCOL.md`。
