#include "xil_types.h"
#include "xparameters.h"
#include <stdio.h>
#include "xil_printf.h"

#define MBOX_0_ADDR XPAR_MBOX_0_BASEADDR // w
#define MBOX_1_ADDR XPAR_MBOX_1_BASEADDR // r


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
int cpuId = XPAR_CPU_ID;


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
    int r = ((*time_seed ^ 32214563) + id) % 3;
    return r;
}

// 1 2 2, 2 3 3, 3 1 1
// 2 1 2, 3 2 3, 1 3 1
// 2 2 1, 3 3 2, 1 1 3



void Game(int *x) {


	MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
	round_winner[*round_index] = -2;
	MutexUnlock(MUTEX_ADDR, 0, cpuId);


	int win = 0;
    int draw = 0;

    if ((x[0] == 1 && x[1] == 2 && x[2] == 2) || (x[0] == 2 && x[1] == 3 && x[2] == 3) ||
        (x[0] == 3 && x[1] == 1 && x[2] == 1)) {

        win = 1;
    }

    if (win) {
        MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
        round_winner[*round_index] = cpuId;
        *round_index += 1;
        MutexUnlock(MUTEX_ADDR, 0, cpuId);

        MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
		xil_printf("P0: Am castigat runda\r\n");
		MutexUnlock(MUTEX_ADDR, 10, cpuId);
    }

    MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
    if (win == 0 && !((x[0] == 2 && x[1] == 1 && x[2] == 2) || (x[0] == 3 && x[1] == 2 && x[2] == 3) || (x[0] == 1 && x[1] == 3 && x[2] == 1))
    		&& !((x[0] ==2 && x[1] == 2 && x[2] == 1) || (x[0] == 3 && x[1] == 3 && x[2] == 2) || (x[0] == 1 && x[1] == 1 && x[2] == 3))) {

    	MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
		round_winner[*round_index] = -1;
		*round_index += 1;
		MutexUnlock(MUTEX_ADDR, 0, cpuId);


    	MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
		xil_printf("P0: Runda incheiata la egalitate.\r\n");
		MutexUnlock(MUTEX_ADDR, 10, cpuId);
    }
	MutexUnlock(MUTEX_ADDR, 0, cpuId);

}

void PrintResults() {
	MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
	int nr = *round_index;
	MutexUnlock(MUTEX_ADDR, 0, cpuId);

	MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
	xil_printf("###########\r\n");
	MutexUnlock(MUTEX_ADDR, 10, cpuId);

	int i, p0 = 0, p1 = 0, p2 = 0, rem = 0;
	for (i = 0; i < nr; i++) {
		MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
		int r = round_winner[i];
		MutexUnlock(MUTEX_ADDR, 0, cpuId);

		switch(r) {
			case 0: p0 += 1; break;
			case 1: p1 += 1; break;
			case 2: p2 += 1; break;
			default: rem += 1; break;
		}
	}

	MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
	xil_printf("p0 : %d castiguri\r\n", p0);
	xil_printf("p1 : %d castiguri\r\n", p1);
	xil_printf("p2 : %d castiguri\r\n", p2);
	xil_printf("remize : %d\r\n", rem);
	MutexUnlock(MUTEX_ADDR, 10, cpuId);

}

#define Q_0_if_0 MBOX_0_ADDR
#define Q_0_if_1 MBOX_1_ADDR

#define Q_1_if_0 XPAR_MBOX_2_BASEADDR
#define Q_1_if_1 XPAR_MBOX_3_BASEADDR

int main()
{

    *time_seed = 1234321;
    *round_index = 0;
//    int sec2 = 200000000;
    int sec2 = 20000000;
    int choice = -1;

    int i, j;
    int rounds = 50;
	TimerReset(TimerBaseAddr, 0);
	TimerStart(TimerBaseAddr, 0);
	while (i < rounds) {
		if (TimerGetCounter(TimerBaseAddr, 0) > sec2) {

			MutexLockBlocking(MUTEX_ADDR, 0, cpuId);
			int index = *round_index;
			MutexUnlock(MUTEX_ADDR, 0, cpuId);


			MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
			xil_printf("P0: Runda %d a inceput.\r\n", index + 1);
			MutexUnlock(MUTEX_ADDR, 10, cpuId);

			int x[3] = {0};
			// make choice
			choice = MakeChoice(cpuId) + 1;
			x[0] = choice;
			PrintChoice(cpuId, choice);

			//

			MboxWriteBlocking(Q_0_if_0, x, 1); // trimite x[0]
			MboxReadBlocking(Q_1_if_0, x + 2, 1); // primeste x[2]
			MboxWriteBlocking(Q_0_if_0, x + 2, 1); // trimite x[2]
			MboxReadBlocking(Q_1_if_0, x + 1, 1); // primeste x[1]


			Game(x);

			*time_seed = TimerGetCounter(TimerBaseAddr, 0);


			for (j = 0; j < 100000; j++) ;
			TimerReset(TimerBaseAddr, 0);
			MutexLockBlocking(MUTEX_ADDR, 10, cpuId);
			xil_printf("P0: Time seed updated. Reset timer.\r\n");
			MutexUnlock(MUTEX_ADDR, 10, cpuId);

			i++;
		}

    }

	PrintResults();


   return 0;
}
