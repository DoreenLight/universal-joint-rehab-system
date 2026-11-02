# SerialConsole — 串口控制台（Qt 6）

「智能康腕系统」上位机：串口连接中间板，下发电机指令、校验帧并记录遥测数据与曲线。

## 入口

| 用途 | 路径 |
| --- | --- |
| CMake 工程 | `CMakeLists.txt`（target：`Uart_controller`，产物 `Uart_controller.exe`） |
| 界面与逻辑 | `mainwindow.cpp` / `mainwindow.ui` / `mainwindow.h` |
| 协议与串口线程 | `protocolworker.cpp` / `protocolworker.h` |

## 依赖

- Qt 6.5+：`Core`、`Widgets`、`SerialPort`、`Charts`（见 `find_package` 声明）
- 构建目录 `build/` 由 Qt Creator 生成（含 MinGW 与 MSVC 多套工具链配置）

## 状态与注意

- 该工程有独立 git 仓库，工作区存在未提交改动（含 `build/` 内生文件）；提交前请确认差异。
- `telemetry_*.csv` 与 `error_frames.txt` 为运行期记录文件，位于 `build/` 各构建目录下。
