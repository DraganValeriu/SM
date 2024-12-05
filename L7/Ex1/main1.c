 #include "xparameters.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_exception.h"

volatile u32* timer_tcsr0_addr;
volatile u32* timer_tcsr0_addr;
volatile u32* timer_tlr0_addr;
volatile u32* timer_tcr0_addr;

volatile u32* intc1_isr_addr;
volatile u32* intc1_ier_addr;
volatile u32* intc1_iar_addr;
volatile u32* intc1_ivr_addr;
volatile u32* intc1_mer_addr;

u32 show_flag = 0;


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
    volatile u32 *mutex_a = (u32 *)(mutexBaseAddr + 256 * mutexNumber);
    *mutex_a = (cpuId << 1);
}
void TimerISR() {
	show_flag = 1;
	// Clear the pending interrupt

	*timer_tcsr0_addr |= (1 << 8);
	*intc1_iar_addr |= XPAR_AXI_TIMER_0_INTERRUPT_MASK;
}


void GlobalISR() { // Interrupt Service Routine
	u32 active_isr = *intc1_ivr_addr;

	switch(active_isr) {
		case XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR:
			TimerISR(); break;
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

int main()
{
	timer_tcsr0_addr = (u32*)(XPAR_TMRCTR_0_BASEADDR);
	timer_tlr0_addr = (u32*)(XPAR_TMRCTR_0_BASEADDR + 0x4);
	timer_tcr0_addr = (u32*)(XPAR_TMRCTR_0_BASEADDR + 0x8);

	intc1_ier_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0x8);

	intc1_iar_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0xC);
	intc1_ivr_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0x18);
	intc1_mer_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0x1C);

	// Set interrupt controller: enable interrupts generated by thetimer
	*intc1_ier_addr |= XPAR_AXI_TIMER_0_INTERRUPT_MASK;
	*intc1_mer_addr = 0x3;

	// Enable microblaze interrupt
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)GlobalISR, 0);
	Xil_ExceptionEnable();

	// Set timer: enable interrupt | auto reload | count down
	*timer_tcsr0_addr = (1 << 6) | (1 << 4) | (1 << 1);
	*timer_tlr0_addr = 150000000; // 1 second
	*timer_tcsr0_addr |= (1 << 7); // Start timer

	while(1) {
		if(show_flag) {
			MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
			xil_printf("P1: 1.5 sec\r\n");
			MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
			show_flag = 0;

		}
	}

	return 0;
}