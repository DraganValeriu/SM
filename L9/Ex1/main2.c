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

volatile u8* data ;
volatile u32* barrier ;
volatile u32* histogram ;

void barrier_sync() {
	MutexLockBlocking(MUTEX_ADDR, 5, XPAR_CPU_ID);
	*barrier += 1;
	MutexUnlock(MUTEX_ADDR, 5, XPAR_CPU_ID);
	while (*barrier < 4) {
		// wait
	}

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

void Init() {
	data = (u8 *)(XPAR_MIG7SERIES_0_BASEADDR + DATA_OFFSET);
	barrier = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + BARRIER_OFFSET);
	histogram = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + HISTOGRAM_OFFSET);
}

int main() {

	Init();

	barrier_sync();

	Histogram();

	barrier_sync();
	xil_printf("end 2\r\n");
	return 0;
}
