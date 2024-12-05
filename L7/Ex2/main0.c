#include "xparameters.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_exception.h"

u32 interrupt_flag;



volatile u32* intc0_isr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR);
volatile u32* intc0_ier_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x8);
volatile u32* intc0_iar_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0xC);
volatile u32* intc0_ivr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x18);
volatile u32* intc0_mer_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x1C);

volatile u32* intc1_isr_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR);


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


void Sw1_ISR() {

    MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
    xil_printf("P0: 0 -> 1\r\n");
    MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);

    TimerStop(TimerBaseAddr, 0);



    interrupt_flag = 1;
// Clear the pending interrupt
    *intc0_iar_addr |= (1 << XPAR_AXI_INTC_0_SW_0_INTR);


}
void GlobalISR() { // Interrupt Service Routine
    u32 active_isr = *intc0_ivr_addr;
    switch (active_isr) {
    case XPAR_AXI_INTC_0_SW_0_INTR:
        Sw1_ISR();
        break;
    case XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR:
    case XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_0_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_1_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_0_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_1_INTR:
    case XPAR_AXI_INTC_0_SW_1_INTR:
    case XPAR_AXI_INTC_0_SW_2_INTR:
    case XPAR_AXI_INTC_0_SW_3_INTR:
    default:
        xil_printf("C%d: No handler for ISR=%x\r\n", XPAR_CPU_ID,
                   active_isr);
        break;
    }
}
int main() {
    int i;
// Set interrupt controller: enable Sw1 interrupts
    *intc0_ier_addr |= (1 << XPAR_AXI_INTC_0_SW_0_INTR);
    *intc0_mer_addr = 0x3;

// Enable microblaze interrupt
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)GlobalISR, 0);
    Xil_ExceptionEnable();
    interrupt_flag = 1;
    // 1s = 100.000.000
    // 3750018, 3749987, 3749983
    // 4999400, 4999407, 4999442


    while (1) {
        if (interrupt_flag) {
            MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
            xil_printf("time = %d\r\n", TimerGetCounter(TimerBaseAddr, 0) );
            MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);

            TimerReset(TimerBaseAddr, 0);

            interrupt_flag = 0;

            TimerStart(TimerBaseAddr, 0);
            // send interrupt to core 1
            *intc1_isr_addr |= (1 << XPAR_AXI_INTC_0_SW_1_INTR);

            //for (i = 0; i < 0xFFFFFF; i++) ;

        }
    }
    Xil_ExceptionDisable();
    return 0;
}
