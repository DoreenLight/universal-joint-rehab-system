static uint8_t frame[29];
static void make_frame(void)
{
    frame[0]=1; frame[1]=3; frame[2]=24;
    for(int i=3;i<27;i++) frame[i]=(uint8_t)(i+10);
    uint16_t crc=Modbus_CalCRC(frame,27);
    frame[27]=(uint8_t)crc;frame[28]=(uint8_t)(crc>>8);
}
static void reset(void)
{
    memset((void*)&g_uart3_diag,0,sizeof(g_uart3_diag));
    memset((void*)&g_uart4_diag,0,sizeof(g_uart4_diag));
    rx_used=dma_position=queue_head=queue_tail=rx_recovery_pending=0;
    u8Uart3InitOk=1;u8Uart3TxBusy=u8Uart3WaitingResponse=u8Uart3ZeroSaveStep=0;
    now=recovery_tick=u32Uart3LastPollTick=u32Uart3RequestTick=0;
    txresult=rxresult=abortresult=HAL_OK;txcalls=aborttx=abortrx=starts=0;
    mask=idle=ipsr=injected_error=0;huart3.Instance=USART3;
}
static void drain(unsigned n)
{
    for(unsigned i=0;i<n;i++){Uart3RxToUart4Task();CHECK(sent_len==29);CHECK(memcmp(sent,frame,29)==0);}
}
int main(void)
{
    make_frame();
    for(int cut=1;cut<29;cut++){
        reset();Uart3Feed(frame,cut);CHECK(g_uart3_diag.valid_frames==0);
        Uart3Feed(frame+cut,29-cut);CHECK(g_uart3_diag.valid_frames==1);drain(1);
    }
    reset();for(int i=0;i<29;i++) Uart3Feed(frame+i,1);drain(1);
    uint8_t joined[58];memcpy(joined,frame,29);memcpy(joined+29,frame,29);
    reset();Uart3Feed(joined,58);CHECK(g_uart3_diag.valid_frames==2);drain(2);
    puts("PASS all split points, bytewise input, concatenated frames, exact forwarded bytes");
    reset();uint8_t bad[29];memcpy(bad,frame,29);bad[28]^=1;
    Uart3Feed(bad,29);CHECK(g_uart3_diag.crc_errors==1);CHECK(g_uart3_diag.valid_frames==0);
    uint8_t noise[]={0,9,1,3,25,255};Uart3Feed(noise,sizeof(noise));Uart3Feed(frame,29);drain(1);
    reset();for(int i=0;i<10;i++)Uart3Feed(frame,29);
    CHECK(g_uart3_diag.valid_frames==10);CHECK(g_uart3_diag.queue_overflows==3);drain(7);
    CHECK(queue_head==queue_tail);
    puts("PASS bad CRC/noise resynchronization and explicit queue overflow accounting");
    reset();
    /* DMA half/full wrap and IDLE must neither duplicate nor lose bytes. */
    for(unsigned i=0;i<1048;i++)u8Uart3RxBuffer[i]=0;
    memcpy(u8Uart3RxBuffer+1030,frame,18);memcpy(u8Uart3RxBuffer,frame+18,11);
    hdma_usart3_rx.remaining=524;DMA1_Channel5_IRQHandler();
    hdma_usart3_rx.remaining=1048;DMA1_Channel5_IRQHandler();
    CHECK(g_uart3_diag.valid_frames==0);
    hdma_usart3_rx.remaining=1037;idle=1;USART3_IRQHandler();
    CHECK(g_uart3_diag.valid_frames==1);CHECK(g_uart3_diag.rx_bytes==1059);drain(1);
    idle=1;USART3_IRQHandler();CHECK(g_uart3_diag.valid_frames==1);
    CHECK(abortrx==0 && starts==0);
    puts("PASS circular DMA half/full/wrap/IDLE, no IDLE abort or restart");
    reset();rx_used=4;injected_error=1;idle=1;USART3_IRQHandler();
    CHECK(rx_recovery_pending==1 && g_uart3_diag.uart_errors==1);
    CHECK(g_uart3_diag.last_uart_error==8);
    now=19;Uart3RecoverRx();CHECK(starts==0);
    now=20;rxresult=HAL_BUSY;Uart3RecoverRx();CHECK(rx_recovery_pending==1);
    CHECK(g_uart3_diag.rx_restart_errors==1);
    now=40;abortresult=HAL_TIMEOUT;Uart3RecoverRx();CHECK(starts==1);
    now=60;abortresult=rxresult=HAL_OK;Uart3RecoverRx();
    CHECK(rx_recovery_pending==0 && rx_used==0 && dma_position==0);
    CHECK(g_uart3_diag.rx_recoveries==1);Uart3Feed(frame,29);drain(1);
    reset();rx_recovery_pending=1;recovery_tick=0xfffffff0U;now=4;Uart3RecoverRx();CHECK(starts==1);
    puts("PASS error recovery, failed abort/rearm retries and tick wrap");
    for(int status=HAL_ERROR;status<=HAL_TIMEOUT;status++){
        reset();txresult=status;Uart3Feed(frame,29);Uart3RxToUart4Task();
        CHECK(txcalls==1 && g_uart3_diag.forward_errors==1);
        CHECK(g_uart4_diag.send_errors==1 && queue_head==queue_tail);
        CHECK(aborttx==(status==HAL_BUSY?0U:1U));
        txresult=HAL_OK;Uart3Feed(frame,29);drain(1);CHECK(g_uart3_diag.forwarded_frames==1);
    }
    CHECK(Uart4SendFrame(NULL,1)==HAL_ERROR);CHECK(Uart4SendFrame(frame,1049)==HAL_ERROR);
    reset();mask=1;CHECK(Uart4SendFrame(frame,29)==HAL_BUSY);CHECK(txcalls==0);
    mask=0;ipsr=1;CHECK(Uart4SendFrame(frame,29)==HAL_BUSY);CHECK(txcalls==0);
    reset();mask=1;Uart3RxToUart4Task();CHECK(mask==1);mask=0;
    u8Uart3TxBusy=1;now=20;Uart3PollTask();CHECK(aborttx==1 && !u8Uart3TxBusy);
    now=40;Uart3PollTask();CHECK(u8Uart3TxBusy && u8Uart3WaitingResponse);
    u8Uart3TxBusy=0;now=60;Uart3PollTask();CHECK(g_uart3_diag.response_timeouts==1);
    puts("PASS bounded forwarding failure/no replay, recovery on later frame, poll timeouts");
    printf("PASS %u assertions; extracted production functions with mocked HAL\n",checks);
    return 0;
}
