#include "xparameters.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_exception.h"

u32 interrupt_flag;

volatile u32* mbox0_if0_addr = (u32*)(XPAR_MBOX_0_BASEADDR);
volatile u32* intc0_isr_addr;
volatile u32* intc0_ier_addr;
volatile u32* intc0_iar_addr;
volatile u32* intc0_ivr_addr;
volatile u32* intc0_mer_addr;


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
void Sw1_ISR() {
	interrupt_flag = 1;
// Clear the pending interrupt
	*intc0_iar_addr |= (1 << XPAR_AXI_INTC_0_SW_1_INTR);
}
void GlobalISR() { // Interrupt Service Routine
	u32 active_isr = *intc0_ivr_addr;
	switch (active_isr) {
	case XPAR_AXI_INTC_0_SW_1_INTR:
		Sw1_ISR();
		break;
	case XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR:
	case XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_0_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_0_INTERRUPT_1_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_0_INTR:
	case XPAR_AXI_INTC_0_MAILBOX_1_INTERRUPT_1_INTR:
	case XPAR_AXI_INTC_0_SW_0_INTR:
	case XPAR_AXI_INTC_0_SW_2_INTR:
	case XPAR_AXI_INTC_0_SW_3_INTR:
	default:
		xil_printf("C%d: No handler for ISR=%x\r\n", XPAR_CPU_ID,
		           active_isr);
		break;
	}
}
int main() {
	intc0_isr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR);
	intc0_ier_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x8);
	intc0_iar_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0xC);
	intc0_ivr_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x18);
	intc0_mer_addr = (u32*)(XPAR_AXI_INTC_0_BASEADDR + 0x1C);
// Set interrupt controller: enable Sw1 interrupts
	*intc0_ier_addr |= (1 << XPAR_AXI_INTC_0_SW_1_INTR);
	*intc0_mer_addr = 0x3;
// Enable microblaze interrupt
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	                             (Xil_ExceptionHandler)GlobalISR, 0);
	Xil_ExceptionEnable();
	interrupt_flag = 1;
	char x[5] = {'1', '2', '3', '4', '5'};
	int i;
	while (1) {
		if (interrupt_flag) {
// Write 5 messages to mbox 0 if 0 - print some logs
			for (i = 0; i < 5; i++) {
				MboxWriteBlocking(MBOX_0_ADDR, x + i, 1);

				MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
				xil_printf("write: %c\r\n", x[i]);
				MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
			}
			interrupt_flag = 0;
		}
	}
	Xil_ExceptionDisable();
	return 0;
}
