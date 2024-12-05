#include "xparameters.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_exception.h"
u32 interrupt_flag = 0;

volatile u32* intc2_isr_addr = (u32*)(XPAR_AXI_INTC_2_BASEADDR);
volatile u32* intc2_ier_addr = (u32*)(XPAR_AXI_INTC_2_BASEADDR + 0x8);
volatile u32* intc2_iar_addr = (u32*)(XPAR_AXI_INTC_2_BASEADDR + 0xC);
volatile u32* intc2_ivr_addr = (u32*)(XPAR_AXI_INTC_2_BASEADDR + 0x18);
volatile u32* intc2_mer_addr = (u32*)(XPAR_AXI_INTC_2_BASEADDR + 0x1C);

volatile u32* intc3_isr_addr = (u32*)(XPAR_AXI_INTC_3_BASEADDR);

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

void Sw1_ISR() {

	MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
	xil_printf("P2: 2 -> 3\r\n");
	MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);


    interrupt_flag = 1;
// Clear the pending interrupt
    *intc2_iar_addr |= (1 << XPAR_AXI_INTC_0_SW_2_INTR);


}
void GlobalISR() { // Interrupt Service Routine
    u32 active_isr = *intc2_ivr_addr;
    switch (active_isr) {
    case XPAR_AXI_INTC_0_SW_2_INTR:
        Sw1_ISR();
        break;
    case XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR:
    case XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_0_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_1_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_0_INTR:
    case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_1_INTR:
    case XPAR_AXI_INTC_0_SW_0_INTR:
    case XPAR_AXI_INTC_0_SW_1_INTR:
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
    *intc2_ier_addr |= (1 << XPAR_AXI_INTC_0_SW_2_INTR);
    *intc2_mer_addr = 0x3;
// Enable microblaze interrupt
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)GlobalISR, 0);
    Xil_ExceptionEnable();

    while (1) {
        if (interrupt_flag) {
            interrupt_flag = 0;

            // send interrupt to core 3
        	*intc3_isr_addr |= (1 << XPAR_AXI_INTC_0_SW_3_INTR);

// wait some random time - adjust it if necessary
             //for (i = 0; i < 0xFFFFFF; i++) ;


        }
    }
    Xil_ExceptionDisable();
    return 0;
}
