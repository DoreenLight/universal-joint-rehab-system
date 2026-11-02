# BleConsole — 蓝牙上位机（Qt 6）

通过 BLE 连接 `Kangfu-RS485`（ESP32-S3 触摸屏），执行腕关节康复训练流程的桌面上位机。

## 入口

| 用途 | 路径 |
| --- | --- |
| CMake 工程 | `CMakeLists.txt`（target：`BleConsole`，产物 `BleConsole.exe`） |
| 界面与流程逻辑 | `mainwindow.cpp` / `mainwindow.ui` / `mainwindow.h` |
| 便携版打包脚本 | `build_portable_release.cmd` → `build/BleConsole_Portable` |
| 阻力版本打包脚本 | `build_resistance_release.cmd` → `build/BleConsole_Resistance` |
| 开发计划与清单 | `docs/plans/2026-07-24-blue-rehab-flow-features.md` |

## 依赖

- Qt 6：`Widgets`、`Bluetooth`（CMake 同时兼容 Qt5 写法）
- Windows 构建：MSVC 2022 + Ninja + `C:\Qt\6.11.1\msvc2022_64`，工具链路径写死在两个 `.cmd` 脚本内，换机需同步修改

## 状态与注意

- 该工程无独立 git 仓库。
- `build/` 内为历史编译产物，本次整理只把其中的 `wanguanjieBLE*` 改名为了 `BleConsole*`，**未重新编译**；下次在 Windows 上重新配置构建会生成同名新产物。
- 训练流程现状以 `docs/plans/` 清单为准：被动模式已接入参数下发与启停接口，主动模式标记为「流程预览」，不下发未经验证的力矩。
