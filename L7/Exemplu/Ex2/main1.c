#include "xparameters.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_exception.h"
u32 interrupt_flag = 0;
volatile u32* mbox0_if1_rit_addr = (u32*)(XPAR_MBOX_1_BASEADDR + 0x1C);
volatile u32* mbox0_if1_ie_addr = (u32*)(XPAR_MBOX_1_BASEADDR + 0x24);
volatile u32* mbox0_if1_is_addr = (u32*)(XPAR_MBOX_1_BASEADDR + 0x20);
volatile u32* intc0_isr_addr;
volatile u32* intc1_isr_addr;
volatile u32* intc1_ier_addr;
volatile u32* intc1_iar_addr;
volatile u32* intc1_ivr_addr;
volatile u32* intc1_mer_addr;

#define MBOX_0_ADDR XPAR_MBOX_0_BASEADDR // w
#define MBOX_1_ADDR XPAR_MBOX_1_BASEADDR // r

#define STATUS_MBOX (MBOX_ADDR + 0x10)

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

void Mbox0if1_ISR() {
// Read 5 messages from mbox 0 if 1
	int i;
	char x[5];
	for (i = 0; i < 5; i++) {
		MboxReadBlocking(MBOX_1_ADDR, x + i, 1);
		MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
		xil_printf("read: %c\r\n", x[i]);
		MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
	}
	interrupt_flag = 1;
// Clear the pending interrupt
	*mbox0_if1_is_addr |= (1 << 1);
	*intc1_iar_addr |= XPAR_MAILBOX_0_INTERRUPT_1_MASK;
}
void GlobalISR() { // Interrupt Service Routine
	u32 active_isr = *intc1_ivr_addr;
	switch (active_isr) {
	case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_1_INTR:
		Mbox0if1_ISR();
		break;
	case XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR:
	case XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_0_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_0_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_1_INTR:
	case XPAR_AXI_INTC_0_SW_0_INTR:
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
	intc0_isr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR);
	intc1_isr_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR);
	intc1_ier_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0x8);
	intc1_iar_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0xC);
	intc1_ivr_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0x18);
	intc1_mer_addr = (u32*)(XPAR_AXI_INTC_1_BASEADDR + 0x1C);
// Set mailbox: set threshold for receive queue and enable interrupts
	*mbox0_if1_rit_addr = 4;
	*mbox0_if1_ie_addr |= (1 << 1);
// Set interrupt controller: enable interrupts generated by the timer
	*intc1_ier_addr |= XPAR_MAILBOX_0_INTERRUPT_1_MASK;
	*intc1_mer_addr = 0x3;
// Enable microblaze interrupt
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	                             (Xil_ExceptionHandler)GlobalISR, 0);
	Xil_ExceptionEnable();
	while (1) {
		if (interrupt_flag) {
// print some logs
			MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
			xil_printf("wait interrupt to finish\r\n");
			MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
// wait some random time - adjust it if necessary
			for (i = 0; i < 0xFFFFFF; i++)
				;
// Send a software interrupt to core 0
			*intc0_isr_addr |= (1 << XPAR_AXI_INTC_0_SW_1_INTR);
			interrupt_flag = 0;
		}
	}
	Xil_ExceptionDisable();
	return 0;
}