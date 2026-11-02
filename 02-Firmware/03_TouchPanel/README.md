# TouchPanel — ESP32-S3 触摸屏固件

万向节康复设备的 4.3 寸触摸屏：本地界面操作 + BLE 服务，把与 Qt 上位机一致的电机控制帧经 RS485 发给中间板。

## 入口

| 用途 | 路径 |
| --- | --- |
| Arduino 主程序 | `03_TouchPanel.ino`（Arduino 要求目录名与 `.ino` 同名，改名时需同步改两者） |
| 界面源码 | `ui.c` / `ui_Screen1.c` / `ui_helpers.c`（由 SquareLine Studio 1.6.1 生成，LVGL 8.3.11） |
| 板级配置 | `esp_panel_board_custom_conf.h`（板型：`Waveshare:ESP32-S3-Touch-LCD-4.3`） |
| 显示与 LVGL 适配 | `lvgl_v8_port.cpp` / `.h` |
| RS485 协议 | `RS485_PROTOCOL.md` |
| 上位机侧同名协议实现 | `03-HostApp/02_SerialReader/UART/MotorUartProtocol.cpp` |

## 链路

- **BLE**：广播名 `Kangfu-RS485`，服务与收发特征 UUID 定义在 `.ino` 顶部；对应上位机为 `03-HostApp/03_BleConsole`。
- **RS485**：115200 8N1，默认 RX = GPIO15、TX = GPIO16，DE/RE 默认关闭（`-1`）；帧格式与 Qt 侧一致（帧头 `0x6D 0x6A`，校验为字节 2–12 之和低 8 位）。

## 依赖

- Arduino IDE（ESP32-S3 开发板支持）+ `ESP32_Display_Panel` + LVGL 8 + FreeRTOS/BLE 组件

## 状态与注意

- `*.backup_*` 为改名前的历史备份文件，保留供追溯。
- `qtconnectivity-Windows-*.7z`（Qt 模块离线包）与本工程无关，可移出。
- `filelist.txt` 与 `CMakeLists.txt` 为界面源码清单，供构建引用。
