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

#define DATA_SIZE SIZE_100KB

int nr = DATA_SIZE >> 2; //
int idx = XPAR_CPU_ID;


#define DATA_OFFSET ((DATA_SIZE >> 2) * idx)
#define BARRIER_OFFSET (DATA_SIZE + 10)
#define SORTED_DATA_OFFSET (DATA_SIZE + DATA_SIZE)




volatile u32* data ;
volatile u32* barrier ;
volatile u32* sorted_data;

void Init() {
	data = (XPAR_MIG7SERIES_0_BASEADDR + DATA_OFFSET);
	barrier = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + BARRIER_OFFSET);
	sorted_data = (u32 *)(XPAR_MIG7SERIES_0_BASEADDR + SORTED_DATA_OFFSET);
}

#define DATA(i) (*((u32 *)(data + (i))))

void InitDdr2(int size_to_init) {

	int i;
// Start timer | Auto reload
	*(u32*)(XPAR_AXI_TIMER_0_BASEADDR + 0x0) |= (1 << 7) | (1 << 4);
	u32 random_seed = *(u32*)(XPAR_AXI_TIMER_0_BASEADDR + 0x8);

	for (i = 0; i < size_to_init; i++ ) {

		random_seed = random_seed * 1664525 + 1013904223;
		//xil_printf("%u \r\n", random_seed);

		*((u32*)(XPAR_MIG7SERIES_0_BASEADDR + i)) = random_seed;
	}
// Print only to check data validity (!= 0x0)
 //xil_printf("%6d: %2x  %d \r\n", i, *((u8*)(XPAR_MIG7SERIES_0_BASEADDR +i)), *((u8*)(XPAR_MIG7SERIES_0_BASEADDR +i)));


// Stop timer
	*(u32*)(XPAR_AXI_TIMER_0_BASEADDR + 0x0) &= ~(1 << 7);
}



void barrier_sync() {
	MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
	*barrier += 1;
	MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
	while (*barrier < 2) {
		// wait
	}
	*barrier = 0;
}

void swap(u32 *a, u32 *b) {
    *a=*a + *b;
    *b=*a - *b;
    *a=*a - *b;
    return ;

}



// Function definition of sort array using shell sort

void ShellSort(u32 nums)
{
    // i -> gap/interval
    for (int i = nums / 2; i > 0; i = i / 2)
    {
    	//xil_printf("%d\r\n", i);
        // Traverse j till we reach the end of the sublist.
        for (int j = i; j < nums; j++)
        {
            for(int k = j - i; k >= 0; k = k - i)
            {
            	if(DATA(k + i) >= DATA(k))
                {
                   break;
                }
                else
                {
                    swap(DATA(k + i), DATA(k));
                }
            }
        }
    }

}

 

void interclasare(u32 *a, u32 *b, u32 *c, u32 *d, int n) {
	u32 *r = sorted_data;
	int ia = 0, ib = 0, ic = 0, id = 0, ir = 0;

	while (ia < n || ib < n || ic < n || id < n) {
		if (ia < n && (a[ia] <= b[ib] && a[ia] <= c[ic] && ia <= d[id])) {
			r[ir++] = a[ia++];
		} else if (ib < n && (b[ib] <= a[ia] && b[ib] <= c[ic] && b[ib] <= d[id])) {
			r[ir++] = b[ib++];
		} else if (ic < n && (c[ic] <= a[ia] && c[ic] <= b[ib] && c[ic] <= d[id])) {
			r[ir++] = c[ic++];
		} else if (id < n && (d[id] <= a[ia] && d[id] <= b[ib] && d[id] <= c[ic])) {
			r[ir++] = d[id++];
		}
	}
}

int check(u32 n) {

	int i;
	for (i = 1; i < n; i++) {
		//xil_printf("%u \r\n", a[i]);
		if (DATA(i + 1) > DATA(i)) {
			return i;
		}
	}
	return 0;
}

void PrintArray(u32 *a, int nr) {
	int i;
	for (i = 0; i < nr; i++) {
		xil_printf("a[%d] = %u\r\n", i, a[i]);
	}
}

void WriteLog(char s[] ) {
	MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
	xil_printf("P0: %s\r\n", s);
	MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
}
void WriteLogWithNumber(char s[], u32 nr) {
	MutexLockBlocking(MUTEX_ADDR, 0, XPAR_CPU_ID);
	xil_printf("P0: %s %u\r\n", s, nr);
	MutexUnlock(MUTEX_ADDR, 0, XPAR_CPU_ID);
}
int main() {

	Init();

	*barrier = 0;

	if (data == NULL) {
		WriteLog("data nu a fost initializat");
	    return -1;
	}

	InitDdr2(DATA_SIZE);
	WriteLog("Data init completed");

	barrier_sync();
	WriteLog("Barrier sync nr. 1");

	TimerReset(TimerBaseAddr, 0);
	TimerStart(TimerBaseAddr, 0);

	ShellSort(data, nr);
	WriteLog("Data sort completed");


	barrier_sync();
	WriteLog("Barrier sync nr. 2");

	TimerStop(TimerBaseAddr, 0);
	WriteLogWithNumber("Time = ", TimerGetCounter(TimerBaseAddr, 0));


	int ok = check(nr * 2);
	if (ok) {
		WriteLogWithNumber("Datele nu au fost sortate. Idx = ", ok);
	} else {
		WriteLog("Succes !!!");
	}

	//PrintArray(data, 100);


	//barrier_sync();


	return 0;
}
