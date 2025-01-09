#include "xparameters.h"
#include "xil_printf.h"


#define TimerBaseAddr XPAR_AXI_TIMER_0_BASEADDR

void TimerSetLoadReg(u32 timerBaseAddr, u32 timerNumber, u32 loadValue)
{
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber) + 0x4) = loadValue;
}

void TimerReset(u32 timerBaseAddr, u32 timerNumber)
{
    TimerSetLoadReg(timerBaseAddr, 0, 0);
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) |= (1 << 5);
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) &= ~(1 << 5);
}

void TimerStart(u32 timerBaseAddr, u32 timerNumber)
{
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) |= (1 << 7);
}

void TimerStop(u32 timerBaseAddr, u32 timerNumber)
{
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) &= ~(1 << 7);
}

u32 TimerGetStatus(u32 timerBaseAddr, u32 timerNumber) {
    return *(u32 *)(timerBaseAddr + (0x10 * timerNumber));
}

u32 TimerGetCounter(u32 timerBaseAddr, u32 timerNumber)
{
    return *(u32 *)(timerBaseAddr + (0x10 * timerNumber) + 0x8);
}

void TimerEnableInterrupt(u32 timerBaseAddr, u32 timerNumber)
{
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) |= (1 << 6);
}

void TimerDisableInterrupt(u32 timerBaseAddr, u32 timerNumber)
{
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) &= ~(1 << 6);
}

void TimerCountDown(u32 timerBaseAddr, u32 timerNumber)
{
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) |= (1 << 1);
}

volatile u32* MUTEX_ADDR = (u32 *)0x43400000;

void MutexLockBlocking(u32 mutexBaseAddr, u32 mutexNumber, u32 cpuId) {
    int i=1;
    volatile u32 *mutex_a = (u32 *)(mutexBaseAddr + 256 * mutexNumber);
    while (i) {
        *mutex_a = (cpuId << 1) + 1;
        if (*mutex_a == ((cpuId << 1) + 1)) {
            i=0;
        }
    }
}

void MutexUnlock(u32 mutexBaseAddr, u32 mutexNumber, u32 cpuId) {
    volatile u32 *mutex_a = (u32 *)(MUTEX_ADDR + 256 * mutexNumber);
    *mutex_a = (cpuId << 1);
}


#define MBOX_0_ADDR XPAR_MBOX_0_BASEADDR
#define MBOX_1_ADDR XPAR_MBOX_1_BASEADDR

int MboxIsEmpty(int MboxIfBaseAddr)
{
    return (*(volatile u32 *)(MboxIfBaseAddr + 0x10) & 1);
}

int MboxIsFull(int MboxIfBaseAddr)
{
    return (*(volatile u32 *)(MboxIfBaseAddr + 0x10) & 2);
}

int MboxFlushReceive(int MboxIfBaseAddr)
{
    *(volatile u32 *)(MboxIfBaseAddr + 0x2c) |= 2;
    return MboxIsEmpty(MboxIfBaseAddr);
}

void MboxReadBlocking(int MboxIfBaseAddr, int *destDataPtr, int dataLen)
{

    int i;
    for (i = 0; i < dataLen; i++)
    {
        while (MboxIsEmpty(MboxIfBaseAddr))
        {
        }
        *(destDataPtr + i) = *(volatile u32 *)(MboxIfBaseAddr + 0x8);
    }
}

void MboxWriteBlocking(int MboxIfBaseAddr, int *srcDataPtr, int dataLen)
{
    int i;
    for (i = 0; i < dataLen; i++)
    {
        while (MboxIsFull(MboxIfBaseAddr))
        {
        }
        *(volatile u32 *)(MboxIfBaseAddr) = *(srcDataPtr + i);
    }
}



void WriteLog(char s[] ) {
    MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
    xil_printf("P%d: %s\r\n",XPAR_CPU_ID ,s);
    MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
}
void WriteLogWithNumber(char s[], u32 nr) {
    MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
    xil_printf("P%d: %s %u\r\n",XPAR_CPU_ID ,s, nr);
    MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
}

void WriteLogWithNumber2(char s[], u32 nr1, u32 n32) {
    MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
    xil_printf("P%d: %s %u -> %u\r\n",XPAR_CPU_ID ,s, nr1, n32);
    MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
}
//---------------------------------------

#define DDR2 XPAR_MIG7SERIES_0_BASEADDR

#define Q_0_if_0 XPAR_MBOX_0_BASEADDR
#define Q_0_if_1 XPAR_MBOX_1_BASEADDR

#define Q_1_if_0 XPAR_MBOX_2_BASEADDR
#define Q_1_if_1 XPAR_MBOX_3_BASEADDR

volatile u32 *shared_counter = (u32 *) DDR2;

int main() {
    int *data;
    int temp_shared_counter_old, temp_shared_counter_new ;
    while(1) {
        MboxReadBlocking(Q_0_if_1, data, 1);

        MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
        temp_shared_counter_old = *shared_counter;
        *shared_counter += 1;
        temp_shared_counter_new = *shared_counter;
        MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);

        WriteLogWithNumber2("shared_counter incremented", temp_shared_counter_old, temp_shared_counter_new);
    }

    return 0;
}
