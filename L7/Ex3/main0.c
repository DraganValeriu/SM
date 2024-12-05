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

#define XPAR_UARTLITE_0_BASEADDR 0x40600000U
#define TX_ADDR (XPAR_UARTLITE_0_BASEADDR + 0x04)

#define STATUS_ADDR (XPAR_UARTLITE_0_BASEADDR + 0x08)
#define RX_ADDR XPAR_UARTLITE_0_BASEADDR
#define BUFFER_SIZE 5

volatile u32 *ctrl_reg =  (u32*)(XPAR_UARTLITE_0_BASEADDR + 0x0C);

void Sw1_ISR() {

    u32 status = *(volatile u32*)(STATUS_ADDR);

    if (status & 1) {
        char c = (char)(*(volatile u32*)(RX_ADDR));

        xil_printf("%c\r\n", c);
       
     }

    *intc0_iar_addr |= (1 << XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR);

}
void GlobalISR() { // Interrupt Service Routine
    u32 active_isr = *intc0_ivr_addr;
    switch (active_isr) {
    case XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR:
            Sw1_ISR();
        break;
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
    *intc0_ier_addr |= (1 << XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR);
    *intc0_mer_addr = 0x3;

// Enable microblaze interrupt
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)GlobalISR, 0);
    Xil_ExceptionEnable();

    *ctrl_reg |= (1 << 4);

    while (1) {

    }
    Xil_ExceptionDisable();
    return 0;
}
