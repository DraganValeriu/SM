#include "xparameters.h"
#include "xil_printf.h"


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
#define SIZE_10MB 0xA00000 // 5 s to initialize
#define SIZE_1MB 0x100000
#define SIZE_100KB 0x19000
#define SIZE 0x10

#define DATA_SIZE SIZE_1MB

int nr = DATA_SIZE >> 2; //
int nr_h = 16;
int idx = XPAR_CPU_ID;

#define DATA_OFFSET ((DATA_SIZE >> 2) * idx)
#define BARRIER_OFFSET DATA_SIZE
#define HISTOGRAM_OFFSET (BARRIER_OFFSET + 1)

volatile u8* data;
volatile u32* barrier;
volatile u32* histogram ;

void InitDdr2(int size_to_init) {

	int i;
// Start timer | Auto reload
	*(u32*)(XPAR_AXI_TIMER_0_BASEADDR + 0x0) |= (1 << 7) | (1 << 4);
	for (i = 0; i < size_to_init; i++) {
		//
		//*((u32*)(XPAR_MIG7SERIES_0_BASEADDR + i)) = 0;
		//
		*((u8*)(XPAR_MIG7SERIES_0_BASEADDR + i)) =
		    *((u8*)(XPAR_AXI_TIMER_0_BASEADDR + 0x8));
// Print only to check data validity (!= 0x0)
 //xil_printf("%6d: %2x  %d \r\n", i, *((u8*)(XPAR_MIG7SERIES_0_BASEADDR +i)), *((u8*)(XPAR_MIG7SERIES_0_BASEADDR +i)));


	}
// Stop timer
	*(u32*)(XPAR_AXI_TIMER_0_BASEADDR + 0x0) &= ~(1 << 7);
}


void InitHist() {
	int i;
	for (i = 0; i < nr_h; i++) {
		histogram[i] = 0;
	}
}



void barrier_sync() {
	MutexLockBlocking(MUTEX_ADDR, 5, XPAR_CPU_ID);
	*barrier += 1;
	MutexUnlock(MUTEX_ADDR, 5, XPAR_CPU_ID);
	while (*barrier < 4) {
		// wait
	}
	*barrier = 0;


}


u8 det_interval(u8 x) {
	return x / nr_h;
}

void Histogram() {
	int i;
	for ( i = 0; i < nr; i++) {
		//xil_printf("%d %d\r\n", *(data + i), det_interval(*(data + i)));
		MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
		histogram[det_interval(*(data + i))] += 1;
		MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
	}


}


void PrintHistogram() {

	int i, j, k = 150000;
	histogram[2] += k;
	histogram[8] += k;
	for (i = 0; i < nr_h; i++) {
		xil_printf("%6d ", histogram[i]);
		histogram[i] = (histogram[i] * 100) / (DATA_SIZE + 2 * k);
	}
	xil_printf("\r\n");
	int s = 0;
	xil_printf("procentaj = \r\n");
	for (i = 0; i < nr_h; i++) {
		xil_printf("%5d%% ", histogram[i]);
		s += histogram[i];
	}
	xil_printf(" totatl = %d\r\n", s);

	for (i = 0; i < nr_h; i++) {
		//histogram[i] = histogram[i] / 10 + (histogram[i] % 10 > 0 && histogram[i] / 10 > 0) ;
		histogram[i] = histogram[i] / 10 + (histogram[i] % 10 >= 5);
	}
	for (i = 0; i < nr_h; i++) {
		xil_printf("%d ", histogram[i]);
	}
	xil_printf("\r\n");

	for (i = 0; i < 9; i++) {
		xil_printf("%d", 10 - i - 1);
		for (j = 0; j < nr_h; j++) {
			if (histogram[j] % 10 > 0 && histogram[j] >= (9 - i)) {
				xil_printf(" #");
			} else {
				xil_printf("  ");
			}
		}
		xil_printf("\r\n");
	}

	xil_printf("+");
	for (j = 0; j < nr_h; j++) {
		xil_printf(" -");
	}
	printf("\r\n");
}


void Init() {
	data = (u8 *)(XPAR_MIG7SERIES_0_BASEADDR + DATA_OFFSET);
	barrier = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + BARRIER_OFFSET);
	histogram = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + HISTOGRAM_OFFSET);

	InitDdr2(nr);

	InitHist();

	*barrier = 0;
}

int main() {

	// 1MB toate proc - 601.923.849 - 6s

	Init();

	barrier_sync(); 	xil_printf("sync 1\r\n");

	TimerReset(TimerBaseAddr, 0);
	TimerStart(TimerBaseAddr, 0);

	Histogram();

	barrier_sync(); 	xil_printf("sync 2\r\n");

	TimerStop(TimerBaseAddr, 0);
	xil_printf("time = %d\r\n", TimerGetCounter(TimerBaseAddr, 0));


	PrintHistogram();
	xil_printf("end 0\r\n");
	return 0;
}
