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
    volatile u32 *mutex_a = (u32 *)(MUTEX_ADDR + 256 * mutexNumber);
    *mutex_a = (cpuId << 1);
}

 
#define SIZE_10MB 0xA00000 // 5 s to initialize
#define SIZE_1MB 0x100000
#define SIZE 0x10

#define DATA_SIZE SIZE_10MB

int nr = DATA_SIZE >> 2; //  
int nr_h = 16;
int idx = XPAR_CPU_ID;

u32 DATA_OFFSET = (DATA_SIZE >> 2) * idx;
u32 BARRIER_OFFSET = DATA_SIZE;
u32 HISTOGRAM_OFFSET = BARRIER_OFFSET + 1;

volatile u8* data = (u8 *)(XPAR_MIG7SERIES_0_BASEADDR + DATA_OFFSET);
volatile u32* barrier = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + BARRIER_OFFSET);
volatile u32* histogram = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + HISTOGRAM_OFFSET);

void barrier_sync() {
	*barrier += 1;
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

int main() {

	//barrier_sync();

	Histogram();

	//barrier_sync();

	return 0;
}
