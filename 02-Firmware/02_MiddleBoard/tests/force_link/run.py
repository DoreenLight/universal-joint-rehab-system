"""Compile extracted production logic with fault-injecting HAL mocks, no hardware."""
from pathlib import Path
import re, subprocess, hashlib, json, difflib

here = Path(__file__).resolve().parent
board = here.parents[1]
root = board.parents[1]
driver = board/'Driver/USART'
s3 = (driver/'usart3.c').read_text()
s4 = (driver/'usart4.c').read_text()
def function(source, name):
    m = re.search(r'(?:static\s+)?(?:void|uint8_t|HAL_StatusTypeDef)\s+'+name+r'\([^;]*?\)\s*\{', source)
    assert m, name
    pos, depth = m.end(), 1
    while depth:
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[m.start():pos]+'\n'

head = '''#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef int HAL_StatusTypeDef;
enum {HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT};
typedef struct {int Instance; uint32_t ErrorCode;} UART_HandleTypeDef;
typedef struct {uint16_t remaining;} DMA_HandleTypeDef;
#define USART3 3
#define UART4 4
#define USART3_IRQn 3
#define DMA1_Channel5_IRQn 5
#define UART_FLAG_IDLE 1
#define RESET 0
static uint32_t now, mask, idle, ipsr;
static unsigned checks, txcalls, aborttx, abortrx, starts;
static int txresult, rxresult, abortresult, injected_error;
static uint8_t sent[1048]; static uint16_t sent_len;
#define CHECK(x) do {++checks; if(!(x)){fprintf(stderr,"FAIL line %d: %s\\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t HAL_GetTick(void){return now;}
static uint32_t __get_PRIMASK(void){return mask;}
static uint32_t __get_IPSR(void){return ipsr;}
static void __disable_irq(void){mask=1;}
static void __enable_irq(void){mask=0;}
static void HAL_NVIC_DisableIRQ(int n){(void)n;}
static void HAL_NVIC_EnableIRQ(int n){(void)n;}
#define __HAL_DMA_GET_COUNTER(h) ((h)->remaining)
#define __HAL_UART_GET_FLAG(h,f) (idle)
#define __HAL_UART_CLEAR_IDLEFLAG(h) (idle=0)
#define Uart3_RS485_RX_ENABLE() ((void)0)
#define Uart3_RS485_TX_ENABLE() ((void)0)
static int HAL_UART_AbortReceive(UART_HandleTypeDef*h){(void)h;++abortrx;return abortresult;}
static int HAL_UART_Receive_DMA(UART_HandleTypeDef*h,uint8_t*d,uint16_t n){(void)h;(void)d;(void)n;++starts;return rxresult;}
static int HAL_UART_AbortTransmit(UART_HandleTypeDef*h){(void)h;++aborttx;return HAL_OK;}
static int HAL_UART_Transmit(UART_HandleTypeDef*h,uint8_t*d,uint16_t n,uint32_t t){(void)h;CHECK(t==100);++txcalls;memcpy(sent,d,n);sent_len=n;return txresult;}
static int HAL_UART_Transmit_DMA(UART_HandleTypeDef*h,uint8_t*d,uint16_t n){(void)h;(void)d;(void)n;return txresult;}
void HAL_UART_ErrorCallback(UART_HandleTypeDef*h);
static void HAL_UART_IRQHandler(UART_HandleTypeDef*h){if(injected_error){h->ErrorCode=8;HAL_UART_ErrorCallback(h);injected_error=0;}}
static void HAL_DMA_IRQHandler(DMA_HandleTypeDef*h){(void)h;}
static uint16_t Modbus_CalCRC(uint8_t*d,uint16_t n){uint16_t c=0xffff;while(n--){c^=*d++;for(int i=0;i<8;i++) c=(c>>1)^((c&1)?0xa001:0);}return c;}
'''
headers = '\n'.join((driver/n).read_text().replace('#include "main.h"','') for n in ['usart3.h','usart4.h'])
state = s3[s3.index('uint8_t u8Uart3TxBuffer'):s3.index('void Uart3Init(')]
parts = [head, headers, state, 'UART_HandleTypeDef huart4; uint8_t u8Uart4TxBuffer[1048]; volatile Uart4Diagnostics g_uart4_diag;']
names = ['Uart3IsModbusResponse','Uart3AppendModbusCrc','Uart3Feed','Uart3ConsumeDma','HAL_UART_ErrorCallback','Uart3RecoverRx','Uart3RxToUart4Task','Uart3RS485SendFrame','Uart3ModbusReadHoldingRegisters','Uart3ModbusWriteMultipleRegisters','Uart3DR304ZeroAll','Uart3DR304SaveParameters','Uart3PollTask','DMA1_Channel5_IRQHandler','USART3_IRQHandler']
parts += [function(s3,n) for n in names]
parts += [function(s4,'Uart4SendFrame'), (here/'test.c').read_text()]
build=here/'build';build.mkdir(exist_ok=True)
(build/'test.c').write_text('\n'.join(parts))
gcc=Path('C:/Qt/Tools/mingw1310_64/bin/gcc.exe')
subprocess.run([str(gcc),'-std=c99','-O2','-Wall','-Wextra','-Werror',str(build/'test.c'),'-o',str(build/'test.exe')],check=True)
out=subprocess.run([str(build/'test.exe')],capture_output=True,text=True,check=True).stdout
# Exact boundary check across all firmware C/H files, including motor and safety.
baseline=json.loads((here/'baseline.json').read_text())
changed=[name for name,sha in baseline.items() if hashlib.sha256((root/name).read_bytes()).hexdigest()!=sha]
allowed={str(Path('02-Firmware/02_MiddleBoard/Driver/USART')/n) for n in ['usart3.c','usart3.h','usart4.c','usart4.h']}
assert set(changed)<=allowed, changed
assert 'DMA_CIRCULAR' in s3 and 'HAL_UART_DMAStop' not in s3
assert 'while (' not in s4 and 'Uart4RegisterSend' not in s4
assert 'DMA1_Channel5_IRQn, 4, 0' in s3
patch=''
for n in ['usart3.c','usart3.h','usart4.c','usart4.h']:
    patch+=''.join(difflib.unified_diff((here/(n+'.before')).read_text().splitlines(True),(driver/n).read_text().splitlines(True),fromfile=n+'.before',tofile=n))
(here/'changes.patch').write_text(patch)
out+='PASS: only four allowed firmware files changed; motor/safety sources unchanged.\n'
out+='LIMIT: host mocks only; no ARM build, hardware DMA timing or flashing.\n'
(here/'results.txt').write_text(out)
print(out)
