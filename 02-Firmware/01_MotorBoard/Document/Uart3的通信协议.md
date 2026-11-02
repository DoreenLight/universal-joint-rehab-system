# UART3 串口通信协议 v2.0

## 1. 物理层

| 参数 | 值 |
|------|-----|
| 接口 | UART3 |
| 引脚 | TX = PC10, RX = PC11 |
| 波特率 | 230400 bps |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 硬件流控 | 无 |

## 2. 帧格式 (传输层)

```
┌────────┬────────┬────────┬──────┬────────────┬──────────┐
│ SYNC_H │ SYNC_L │  LEN   │ CMD  │  PAYLOAD   │   SUM    │
│  0x6C  │  0x67  │  1B    │  1B  │  0~250 B   │   1B     │
└────────┴────────┴────────┴──────┴────────────┴──────────┘
```

| 字段 | 偏移 | 字节 | 说明 |
|------|------|------|------|
| SYNC_H | 0 | 1 | 帧头高字节, 固定 `0x6C` ('l') |
| SYNC_L | 1 | 1 | 帧头低字节, 固定 `0x67` ('g') |
| LEN | 2 | 1 | 载荷字节数 (0 ~ 250) |
| CMD | 3 | 1 | 命令码 |
| PAYLOAD | 4 | LEN | 数据载荷, 格式由 CMD 决定 |
| SUM | 4+LEN | 1 | 8位和校验 |

### 校验和计算

```
SUM = (LEN + CMD + PAYLOAD[0] + PAYLOAD[1] + ... + PAYLOAD[LEN-1]) & 0xFF
```

即从 LEN 字段开始, 到 PAYLOAD 最后一个字节结束, 所有字节累加后取低 8 位。

**例**: 发送 `6C 67 02 01  01 01`
- LEN=0x02, CMD=0x01, PAYLOAD=[0x01, 0x01]
- SUM = (0x02 + 0x01 + 0x01 + 0x01) & 0xFF = 0x05
- 完整帧: `6C 67 02 01  01 01  05`

### 帧边界

- 最小帧: 5 字节 (`6C 67 00 xx xx`)
- 最大帧: 255 字节 (`6C 67 FA ... xx`)
- 帧间由空闲间隔分隔 (MCU 端使用 DMA IDLE 检测)

---

## 3. 命令码 (CMD)

### 3.1 上位机 → MCU

| CMD | 名称 | 载荷格式 | 说明 |
|-----|------|---------|------|
| `0x01` | IO_WRITE | `[id:1B] [val:1B]` | 写 IO 信号 |
| `0x02` | FLOAT_WRITE | `[id:1B] [val:4B_LE]` | 写单个浮点数 |
| `0x03` | BATCH_WRITE | `[cnt:1B] [(id:1B)(val:4B_LE)]*` | 批量写浮点数 |
| `0x84` | PING | 空载荷 (LEN=0) | 心跳检测, MCU 自动回复 ACK |
| `0x85` | FLASH_SAVE | 空载荷 (LEN=0) | 将当前 PID 参数写入 Flash |

### 3.2 MCU → 上位机

| CMD | 名称 | 载荷格式 | 说明 |
|-----|------|---------|------|
| `0x81` | TELEMETRY | `[cnt:1B] [(id:1B)(val:4B_LE)]*` | 批量遥测上传 |
| `0x82` | IO_REPORT | `[id:1B] [val:1B]` | IO 状态上报 |
| `0x83` | ACK | `[ack_cmd:1B]` | 应答帧 |

---

## 4. 浮点数编码

**小端序 IEEE 754 单精度浮点数 (4 字节)**。

上位机解析示例:

**Python:**
```python
import struct

def pack_float(val: float) -> bytes:
    return struct.pack('<f', val)

def unpack_float(data: bytes) -> float:
    return struct.unpack('<f', data)[0]
```

**C#:**
```csharp
byte[] PackFloat(float val) => BitConverter.GetBytes(val);
float UnpackFloat(byte[] data) => BitConverter.ToSingle(data, 0);
```

**C/C++ (嵌入式端):**
```c
float unpack_float(const uint8_t *data) {
    uint32_t raw = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    float val;
    memcpy(&val, &raw, 4);
    return val;
}
```

---

## 5. 浮点 ID 定义

| ID | 名称 | 方向 | 说明 |
|----|------|------|------|
| `0x01` | MOTOR1_ANGLE | ↑↓ | 电机1 累计角度 (°) |
| `0x02` | MOTOR1_SPEED | ↑↓ | 电机1 速度 |
| `0x03` | MOTOR1_CURRENT | ↑↓ | 电机1 电流 (mA) |
| `0x04` | MOTOR1_KP | ↑↓ | 电机1 比例系数 Kp |
| `0x05` | MOTOR1_KI | ↑↓ | 电机1 积分系数 Ki |
| `0x11` | MOTOR2_ANGLE | ↑↓ | 电机2 角度 (°) |
| `0x12` | MOTOR2_SPEED | ↑↓ | 电机2 速度 |
| `0x13` | MOTOR2_TORQUE | ↑↓ | 电机2 力矩 |
| `0x14` | MOTOR2_KP | ↑↓ | 电机2 比例系数 Kp |
| `0x15` | MOTOR2_KD | ↑↓ | 电机2 微分系数 Kd |
| `0x21` | SENSOR_ANGLE | ↑ | 传感器角度 (0~360°) |
| `0x22` | SENSOR_CURRENT | ↑ | 传感器电流 (mA) |
| `0x31` | IMU_ROLL | ↑ | IMU 横滚角 (°) |
| `0x32` | IMU_PITCH | ↑ | IMU 俯仰角 (°) |
| `0x33` | IMU_YAW | ↑ | IMU 偏航角 (°) |

> ↑ = MCU 上传, ↓ = 上位机下发, ↑↓ = 双向
>
> **可写浮点** (上位机→MCU, 写入后需发 `0x85` 保存到 Flash)：
> `0x04` (Kp1), `0x05` (Ki1), `0x14` (Kp2), `0x15` (Kd2)

---

## 6. IO ID 定义

| ID | 名称 | 值 | 说明 |
|----|------|-----|------|
| `0x01` | START_STOP | 1 = 启动, 2 = 停止 | 电机启停控制 |
| `0x02` | MODE | 0=定点, 1=正弦, 2=方波, 3=PWM测试 | 运行模式 |
| `0x03` | NEI_WAI | 0=PWM, 1=内部, 2=内外结合, 3=外部 | 内外环选择 |

---

## 7. 实战示例

### 7.1 Ping 心跳

```
上位机发:  6C 67 00 84  84
           ───┬─── ─┬─  ──
            header cmd  SUM(0x00+0x84=0x84)

MCU 回复:  6C 67 01 83  84  88
           ───┬─── ─┬─  ──  ──
            header cmd ack  SUM(0x01+0x83+0x84=0x88)
```

### 7.2 启动电机1

```
上位机发:  6C 67 02 01  01 01  05
           ───┬─── ─┬─  ─┬─  ──
            header cmd  id val  SUM

id=0x01 (START_STOP), val=0x01 (启动)
```

### 7.3 设置电机1 Kp=3.5, Ki=0.2

```
3.5f  → LE bytes: 00 00 60 40
0.2f  → LE bytes: CD CC 4C 3E

解析: 6C 67   0B   03   02  04    00 00 60 40   05    CD CC 4C 3E   XX
      ──┬──   ──   ──   ──  ──   ─────┬─────   ──   ─────┬─────   ──
      header  LEN  CMD  cnt  id1     val1(3.5)   id2   val2(0.2)  SUM

LEN = 0x0B = 11 (1 + 2×5)
SUM = (0x0B+0x03+0x02+0x04+0x00+0x00+0x60+0x40+0x05+0xCD+0xCC+0x4C+0x3E) & 0xFF
    = 0xBE

完整帧: 6C 67 0B 03 02 04 00 00 60 40 05 CD CC 4C 3E BE
```

### 7.4 MCU 上传遥测 (10 个 float)

```
MCU 每秒发送一帧:

6C 67 33 81  0A
01  XX XX XX XX    ← id=0x01 Motor1 角度
02  XX XX XX XX    ← id=0x02 Motor1 速度
03  XX XX XX XX    ← id=0x03 Motor1 电流
04  XX XX XX XX    ← id=0x04 Motor1 Kp
05  XX XX XX XX    ← id=0x05 Motor1 Ki
11  XX XX XX XX    ← id=0x11 Motor2 角度
12  XX XX XX XX    ← id=0x12 Motor2 速度
13  XX XX XX XX    ← id=0x13 Motor2 力矩
14  XX XX XX XX    ← id=0x14 Motor2 Kp
15  XX XX XX XX    ← id=0x15 Motor2 Kd
SUM                ← 8-bit checksum

帧总长: 4(header) + 1(cnt) + 10×5(float) + 1(sum) = 56 字节
```

### 7.5 上位机解析伪代码

```python
def parse_frame(data: bytes):
    if len(data) < 5: return None
    if data[0] != 0x6C or data[1] != 0x67: return None

    length = data[2]
    if length > 250: return None
    if len(data) < 4 + length + 1: return None

    # 验证校验和
    calc_sum = sum(data[2 : 4 + length]) & 0xFF
    rx_sum = data[4 + length]
    if calc_sum != rx_sum: return None

    cmd = data[3]
    payload = data[4 : 4 + length]

    if cmd == 0x81:   # TELEMETRY
        count = payload[0]
        floats = {}
        for i in range(count):
            fid = payload[1 + i*5]
            val = struct.unpack('<f', payload[2+i*5 : 6+i*5])[0]
            floats[fid] = val
        return ('telemetry', floats)

    elif cmd == 0x83:  # ACK
        return ('ack', payload[0])

    # ... 其他 CMD 处理
```

---

## 8. 通信时序

```
上位机                          MCU
  │                              │
  │──── PING ───────────────────→│  任意时刻
  │←─── ACK ────────────────────│  自动回复
  │                              │
  │──── IO_WRITE / FLOAT_WRITE ─→│  任意时刻
  │                              │  立即执行, 不回复 (fire-and-forget)
  │                              │
  │←─── TELEMETRY (10 floats) ──│  每秒 1 次
  │←─── TELEMETRY (10 floats) ──│
  │                              │
```

> IO/浮点写命令采用"发后即忘"模式, 不等待 ACK。如需确认, 上位机可发 PING 检测链路状态。

---

## 9. 错误处理

- **帧头不匹配**: 跳过该字节, 继续搜索 `0x6C`
- **LEN 超出范围** (>250): 丢弃当前帧, 重新同步
- **校验和不匹配**: 丢弃当前帧, MCU 内部计数 `chk_err`
- **DMA 发送忙**: `proto_transport_send()` 返回错误码 `0x04`, 上位机可稍后重试



