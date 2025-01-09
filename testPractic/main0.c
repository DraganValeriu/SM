#include "xparameters.h"
#include "xil_printf.h"
#include "xil_exception.h"

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

void TimerAutoReload(u32 timerBaseAddr, u32 timerNumber) {
    *(u32 *)(timerBaseAddr + (0x10 * timerNumber)) |= (1 << 4);
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


//---------------------------------------

#define DDR2 XPAR_MIG7SERIES_0_BASEADDR

#define Q_0_if_0 XPAR_MBOX_0_BASEADDR
#define Q_0_if_1 XPAR_MBOX_1_BASEADDR

#define Q_1_if_0 XPAR_MBOX_2_BASEADDR
#define Q_1_if_1 XPAR_MBOX_3_BASEADDR

volatile u32 *shared_counter = (u32 *) DDR2;

volatile u32* timer_tcsr0_addr;
volatile u32* timer_tlr0_addr;
volatile u32* timer_tcr0_addr;

volatile u32* intc0_isr_addr;
volatile u32* intc0_ier_addr;
volatile u32* intc0_iar_addr;
volatile u32* intc0_ivr_addr;
volatile u32* intc0_mer_addr;


int intr_flag = 0, seconds = 0;

void TimerISR() {
    intr_flag = 1;
    seconds += 1;

// Clear the pending interrupt
    *timer_tcsr0_addr |= (1 << 8);
    *intc0_iar_addr |= XPAR_AXI_TIMER_0_INTERRUPT_MASK;
}

void GlobalISR() { // Interrupt Service Routine
    u32 active_isr = *intc0_ivr_addr;

    switch (active_isr) {
    case XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR:
        TimerISR();
        break;
    case XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_0_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_1_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_0_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_1_INTR:
    case XPAR_AXI_INTC_0_SW_0_INTR:
    case XPAR_AXI_INTC_0_SW_1_INTR:
    case XPAR_AXI_INTC_0_SW_2_INTR:
    case XPAR_AXI_INTC_0_SW_3_INTR:
    default:
        xil_printf("C%d: No handler for ISR=%x\r\n", XPAR_CPU_ID, active_isr);
        break;
    }
}

int main() {
    timer_tcsr0_addr = (u32*)(XPAR_TMRCTR_0_BASEADDR);
    timer_tlr0_addr = (u32*)(XPAR_TMRCTR_0_BASEADDR + 0x4);
    timer_tcr0_addr = (u32*)(XPAR_TMRCTR_0_BASEADDR + 0x8);

    intc0_isr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR);
    intc0_ier_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x8);
    intc0_iar_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0xC);
    intc0_ivr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x18);
    intc0_mer_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x1C);
    *shared_counter = 0;

    *intc0_ier_addr |= XPAR_AXI_TIMER_0_INTERRUPT_MASK;
    *intc0_mer_addr = 0x3;

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)GlobalISR, 0);
        Xil_ExceptionEnable();

    // Set timer: enable interrupt | auto reload | count down
//
//  *timer_tcsr0_addr = (1 << 6) | (1 << 4) | (1 << 1);
//  *timer_tlr0_addr = 100000000; // 1 second
//  *timer_tcsr0_addr |= (1 << 7); // Start timer
        TimerReset(TimerBaseAddr, 0);
        TimerCountDown(TimerBaseAddr, 0);
        TimerSetLoadReg(TimerBaseAddr, 0, 100000000);
        TimerAutoReload(TimerBaseAddr, 0);
        TimerEnableInterrupt(TimerBaseAddr, 0);
        TimerStart(TimerBaseAddr, 0);
    while (1) {
        if (intr_flag == 1) {
            MboxWriteBlocking(Q_0_if_0, 1111111, 1);
            MboxWriteBlocking(Q_1_if_0, 1111111, 1);
            intr_flag = 0;
            WriteLogWithNumber("seconds passed", seconds);

        }
    }

    return 0;
}
