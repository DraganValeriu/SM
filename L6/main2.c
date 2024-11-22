#include "xil_types.h"
#include "xparameters.h"
#include <stdio.h>
#include "xil_printf.h"

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
    volatile u32 *mutex_a = (u32 *)(mutexBaseAddr + 256 * mutexNumber);
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
// XPAR_AXI_TIMER_0_BASEADDR  0x41C00000

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

// u32 TimerGetStatus(u32 timerBaseAddr, u32 timerNumber) {
//
// }

u32 TimerGetCounter(u32 timerBaseAddr, u32 timerNumber)
{
    return *(u32 *)(timerBaseAddr + (0x10 * timerNumber) + 0x8);
}

volatile u32 *time_seed = (u32 *)0x80000000;
volatile u32 *round_index = (u32 *)(0x80000000 + 0x4);
volatile u32 *round_winner = (u32 *)(0x80000000 + 0x8);

#define TX_ADDR (XPAR_UARTLITE_0_BASEADDR + 0x04)

#define STATUS_ADDR_UART (XPAR_UARTLITE_0_BASEADDR + 0x08)

void SendString(const char * str) {

    int i = 0;
    while (str[i] != '\0') {

        while ((*(volatile u32*)(STATUS_ADDR_UART) & (1 << 3)) != 0) { }

        *(volatile u8*)(TX_ADDR) = str[i];
        i++;

    }
}



void PrintChoice(int cpuId, int choise) {
    const char* ch;
    switch(choise) {
    case 1:
        ch = "Piatra"; break;
    case 2:
        ch = "Foarfece"; break;
    case 3:
        ch = "Hartie"; break;
    default:
        ch = "null"; break;
    }
    MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
    xil_printf("P%d: %s\r\n", cpuId, ch);
    MutexUnlock(MUTEX_ADDR, 10, cpuId);
}


int MakeChoice(int id)
{
    int r = ((*time_seed ^ 23235557)) % 3;
    return r;
}

#define Q_0_if_0 MBOX_0_ADDR
#define Q_0_if_1 MBOX_1_ADDR

#define Q_1_if_0 XPAR_MBOX_2_BASEADDR
#define Q_1_if_1 XPAR_MBOX_3_BASEADDR

int cpuId = XPAR_CPU_ID;

// 1 2 2, 2 3 3, 3 1 1
// 2 1 2, 3 2 3, 1 3 1
// 2 2 1, 3 3 2, 1 1 3

void Game(int *x) {
	int win = 0;

	if ((x[0] ==2 && x[1] == 1 && x[2] == 2) ||
		(x[0] == 3 && x[1] == 2 && x[2] == 3) ||
		(x[0] == 1 && x[1] == 3 && x[2] == 1)) {

		win = 1;
	}

	if (win) {
		MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
		round_winner[*round_index] = cpuId;
		*round_index += 1;
		MutexUnlock(MUTEX_ADDR, 0, cpuId);

		MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
		xil_printf("P1: Am castigat runda\r\n");
		MutexUnlock(MUTEX_ADDR, 10, cpuId);

	}
}

int main()
{


    int i;
    int choice;
    while (1) {
    	int x[3] = {0};

		MboxReadBlocking(Q_0_if_1, x, 1); // primeste x[0]

		choice = MakeChoice(cpuId) + 1;
		x[1] = choice;
		PrintChoice(cpuId, choice);

		MboxWriteBlocking(Q_0_if_1, x + 1, 1); // trimite x[1]
		MboxReadBlocking(Q_0_if_1, x + 2, 1); // primeste x[2]
		MboxWriteBlocking(Q_0_if_1, x, 1); // trimite x[0]

		Game(x);

    }

    return 0;
}
