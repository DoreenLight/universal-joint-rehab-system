"""Read-only production-source extraction; host mocks, no device connection."""
from pathlib import Path
import re, subprocess, hashlib, json

here = Path(__file__).resolve().parent
root = here.parents[3]
motor = root/'02-Firmware/01_MotorBoard/Core/Src/usart.c'
middle = root/'02-Firmware/02_MiddleBoard/Driver/USART/usart3.c'
uart1 = middle.with_name('usart1.c')
paths = [motor, middle, uart1, middle.with_name('usart3.h')]
before = {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
m, b, u = (p.read_text(encoding='utf8', errors='replace') for p in paths[:3])
def function(source, name):
    match = re.search(r'(?:static\s+)?(?:void|uint8_t|HAL_StatusTypeDef)\s+'+name+r'\([^;]*?\)\s*\{', source)
    assert match, name
    start, pos, depth = match.start(), match.end(), 1
    while depth:
        depth += (source[pos]=='{') - (source[pos]=='}')
        pos += 1
    return source[start:pos]+'\n'
parts = ['''#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %d: %s\\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t __get_PRIMASK(void){return 0;}
static void __disable_irq(void){}
static void __enable_irq(void){}
''']
parts += [m[m.index('#define DR304_MODBUS_ADDR'):m.index('void USART2_DR304PrintTask')]]
parts += [m[m.index('static float g_force_zero_offset'):m.index('static void USART2_DebugLogByte')]]
h = paths[3].read_text(encoding='utf8')
parts += ['\n'.join(line for line in h.splitlines() if line.startswith('#define DF_'))+'\n']
parts += ['''typedef int HAL_StatusTypeDef;
#define HAL_OK 0
#define HAL_ERROR 1
#define HAL_BUSY 2
static uint32_t now;
static unsigned reads,writes;
static uint32_t HAL_GetTick(void){return now;}
static void Uart3_RS485_RX_ENABLE(void){}
static HAL_StatusTypeDef Uart3ModbusReadHoldingRegisters(uint8_t a,uint16_t r,uint16_t n){(void)a;(void)r;(void)n;++reads;return HAL_OK;}
static HAL_StatusTypeDef Uart3ModbusWriteMultipleRegisters(uint8_t a,uint16_t r,const uint16_t *v,uint16_t n){(void)a;(void)r;(void)v;(void)n;++writes;return HAL_OK;}
static uint16_t Modbus_CalCRC(uint8_t *b,uint16_t n){return USART2_ModbusCRC16(b,n);}
''']
parts += [b[b.index('static volatile uint8_t u8Uart3WaitingResponse'):b.index('HAL_StatusTypeDef Uart3DmaRxInit')]]
for name in ['Uart3IsModbusResponse','Uart3DR304RequestZeroAllAndSave','Uart3DR304ZeroAll','Uart3DR304SaveParameters','Uart3PollTask']:
    parts += [function(b,name)]
parts += ['\n'.join(line for line in u.splitlines() if line.startswith('#define DF_'))+'\n']
for name in ['Uart1QtFrameChecksum','Uart1IsValidQtFrame','Uart1IsForceSensorZeroCommand']:
    parts += [function(u,name)]
parts += [r'''
static void crc(uint8_t *f,unsigned n){uint16_t c=USART2_ModbusCRC16(f,n-2);f[n-2]=(uint8_t)c;f[n-1]=(uint8_t)(c>>8);}
static void frame(uint8_t *f,int base){unsigned i;f[0]=1;f[1]=3;f[2]=24;for(i=0;i<6;++i){uint32_t v=(uint32_t)(base+(int)i*100);f[3+i*4]=(uint8_t)(v>>24);f[4+i*4]=(uint8_t)(v>>16);f[5+i*4]=(uint8_t)(v>>8);f[6+i*4]=(uint8_t)v;}crc(f,29);}
static void feed(uint8_t *f,unsigned n){unsigned i;for(i=0;i<n;++i)USART2_DR304ProcessByte(f[i]);}
int main(void){
 uint8_t f[29], pair[58],ack[8]={1,0x10,0x0a,0x20,0,2,0,0};
 uint8_t exception[5]={1,0x83,2,0,0};
 uint8_t qtzero[8]={0x6c,0x67,2,1,0x0c,1,0x10,0xed};
 uint8_t legacyzero[14]={0x6d,0x6a,1,0x21,1,1,0,0,0,0,0,0,0,0};
 float out[6];unsigned i;
 frame(f,100);CHECK(Uart3IsModbusResponse(f,29));feed(f,29);
 CHECK(g_dr304_frame_count==1);USART2_DR304GetLatestValues(out);CHECK(fabsf(out[3]-4)<0.0001f);
 USART2_DR304SetZero();
 for(i=0;i<1000;++i){USART2_DR304GetLatestValues(out);CHECK(out[0]==0 && out[3]==0 && out[5]==0);}
 CHECK(g_dr304_frame_count==1);
 puts("PASS: valid nonzero sample -> software zero -> 1000 reads without new frames stay zero");
 frame(f,200);f[28]^=1;CHECK(!Uart3IsModbusResponse(f,29));feed(f,29);
 CHECK(g_dr304_crc_error_count==1);USART2_DR304GetLatestValues(out);CHECK(out[3]==0);
 puts("PASS: bad CRC keeps old zeroed sample (no new valid force frame)");
 frame(f,200);feed(f,29);USART2_DR304GetLatestValues(out);CHECK(fabsf(out[3]-1)<0.0001f);
 CHECK(g_dr304_frame_count==2);
 puts("PASS: next valid changed frame restores changed force; zero does not latch future values to zero");
 CHECK(!Uart3IsModbusResponse(f,10));CHECK(!Uart3IsModbusResponse(f+10,19));
 memcpy(pair,f,29);memcpy(pair+29,f,29);CHECK(!Uart3IsModbusResponse(pair,58));
 puts("PASS: middle validator rejects split or concatenated frames per IDLE chunk");
 crc(ack,8);CHECK(!Uart3IsModbusResponse(ack,8));
 crc(exception,5);CHECK(Uart3IsModbusResponse(exception,5));feed(exception,5);CHECK(g_dr304_frame_count==2);
 puts("PASS: write ACK rejected; read exception accepted by middle but produces no motor force update");
 CHECK(!Uart1IsForceSensorZeroCommand(qtzero,8));
 legacyzero[13]=Uart1QtFrameChecksum(legacyzero);CHECK(Uart1IsForceSensorZeroCommand(legacyzero,14));
 puts("PASS: current Qt zero is NOT the middle board legacy sensor-zero command");
 u8Uart3InitOk=1;CHECK(Uart3DR304RequestZeroAllAndSave()==HAL_OK);
 for(now=0;now<150;++now)Uart3PollTask();
 CHECK(writes==2 && reads>0 && u8Uart3ZeroSaveStep==0);
 printf("PASS: mocked successful sends, no replies: zero state clears and polling resumes (writes=%u reads=%u)\n",writes,reads);
 printf("PASS %u assertions; host extraction only, no MCU/DMA timing or hardware validation\n",checks);
 return 0;
}
''']
build=here/'build';build.mkdir(exist_ok=True)
source=build/'extracted_test.c';source.write_text('\n'.join(parts),encoding='utf8')
exe=build/'extracted_test.exe'
subprocess.run(['C:/Qt/Tools/mingw1310_64/bin/gcc.exe','-std=c99','-O0','-Wall','-Wextra','-Wno-unused-variable',str(source),'-lm','-o',str(exe)],check=True)
result=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
(here/'results.txt').write_text(result.stdout,encoding='utf8');print(result.stdout)
after={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
assert before==after
(here/'source-sha256.json').write_text(json.dumps(before,indent=2),encoding='utf8')
print('PASS extracted production files unchanged')
