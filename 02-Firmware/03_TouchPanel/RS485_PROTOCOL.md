# ESP32 to STM32 RS485 protocol

The ESP32 touch screen now sends the same motor-control frames as the Qt upper-computer code in `03-HostApp/02_SerialReader/UART/MotorUartProtocol.cpp`. The STM32 middle board should receive these frames and forward/translate them exactly as if they came from the Qt serial port.

## Physical layer

- UART: `115200`, `8N1`
- ESP32 default pins in `03_TouchPanel.ino`:
  - RX: GPIO15
  - TX: GPIO16
  - DE/RE: disabled by default (`-1`)

Change `RS485_RX_PIN`, `RS485_TX_PIN`, `RS485_DE_RE_PIN`, and `RS485_BAUD` in `03_TouchPanel.ino` if the wiring is different.

## Qt-compatible frame format

| Byte(s) | Field | Description |
| --- | --- | --- |
| 0 | `0x6D` | Header byte 1 (`m`) |
| 1 | `0x6A` | Header byte 2 (`j`) |
| 2 | `0x01` | Address code |
| 3 | `cmd1` | Command group |
| 4 | `cmd2` | Command index |
| 5..12 | Data | 8 data bytes |
| 13 | Checksum | Sum of bytes 2..12, low 8 bits |

Single-float commands put the float in bytes 5..8 as STM32 little-endian IEEE754 and fill bytes 9..12 with zero. Integer commands put the 32-bit little-endian integer in bytes 5..8 and fill bytes 9..12 with zero.

## Touch command mapping

| Touch control | Qt-equivalent serial command |
| --- | --- |
| Left `Start` | Sends motor 1 angle, velocity, current, KP, KI frames, then motor 1 start |
| Right `Send` | Sends motor 2 MIT dynamic-parameter frame |
| Right `Reset Zero` | Sends motor 2 zero-set frame |
| Right `Start` | Sends motor 2 start frame |

## Command bytes

| Meaning | `cmd1` | `cmd2` | Data |
| --- | --- | --- | --- |
| Motor 1 start/stop | `0x01` | `0x01` | `int32`: `1` start, `0` stop |
| Motor 1 angle | `0x06` | `0x01` | `float` |
| Motor 1 velocity | `0x06` | `0x02` | `float` |
| Motor 1 current | `0x06` | `0x03` | `float` |
| Motor 1 KP | `0x06` | `0x04` | `float` |
| Motor 1 KI | `0x06` | `0x05` | `float` |
| Motor 2 start/stop | `0x11` | `0x01` | `int32`: `1` start, `0` stop |
| Motor 2 zero set | `0x11` | `0x03` | `int32`: `1` |
| Motor 2 MIT params | `0x16` | `0x01` | Packed MIT frame, same as Qt |
| D.R304 force sensor zero | `0x21` | `0x01` | `int32`: non-zero triggers all-channel zero |

Motor 2 MIT parameter packing matches `buildMotorMITDynamicParamsCommand()`:

- Byte 5..6: target angle, `-12.57..12.57 rad` mapped to `uint16`, big-endian.
- Byte 7 and upper nibble of byte 8: target velocity, `-33..33 rad/s` mapped to 12 bits.
- Lower nibble of byte 8 and byte 9: KP, `0..500` mapped to 12 bits.
- Byte 10 and upper nibble of byte 11: KD, `0..5` mapped to 12 bits.
- Lower nibble of byte 11 and byte 12: target torque, `-14..14 Nm` mapped to 12 bits.
