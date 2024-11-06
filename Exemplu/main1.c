#include "xil_types.h" // include tipuri predefinite: u8
#include "xparameters.h" // include constante ce descriu sistemul

#define XPAR_UARTLITE_0_BASEADDR 0x40600000U
#define TX_ADDR (XPAR_UARTLITE_0_BASEADDR + 0x04)

#define STATUS_ADDR_UART (XPAR_UARTLITE_0_BASEADDR + 0x08)
#define RX_ADDR XPAR_UARTLITE_0_BASEADDR
#define BUFFER_SIZE 5


#define MBOX_ADDR XPAR_MBOX_1_BASEADDR
#define STATUS_MBOX (MBOX_ADDR + 0x10)


void SendString(char * str) {

    int i = 0;
    while (str[i] != '\0') {

        while ((*(volatile u32*)(STATUS_ADDR_UART) & (1 << 3)) != 0) { }

        *(volatile u8*)(TX_ADDR) = str[i];
        i++;

    }
}



//—------------------------------------------------------------
// Returneaza valoarea bitului Empty din registrul STATUS
int MboxIsEmpty(int MboxIfBaseAddr) {
	return (*(volatile u32*)(MboxIfBaseAddr + 0x10) & 1);
}
//—------------------------------------------------------------
// Returneaza valoarea registrului Full din registrul STATUS
int MboxIsFull(int MboxIfBaseAddr) {
	return (*(volatile u32*)(MboxIfBaseAddr+ 0x10) & 2);
}
//—------------------------------------------------------------
// Goleste coada de primire a datelor prin scierea bitului CRF din
//registrul CTRL
// Returneaza valoarea bitului Empty din registrul STATUS
int MboxFlushReceive(int MboxIfBaseAddr) {
	*(volatile u32*)(MboxIfBaseAddr+ 0x2c) |= 2;
	return MboxIsEmpty(MboxIfBaseAddr);
}
//—------------------------------------------------------------
// Citeste un sir de lungime dataLen din coada de primire a datelor si
//stocheaza datele primite incepand cu adresa destDataPtr
// Functia este blocanta. Va astepta pana se va citi numarul de intregi
//dorit
void MboxReadBlocking(int MboxIfBaseAddr, int* destDataPtr, int dataLen) {

	int i;
	for (i = 0; i < dataLen; i++) {
		while(MboxIsEmpty(MboxIfBaseAddr))
		{}
		*(destDataPtr + i) = *(volatile u32*)(MboxIfBaseAddr + 0x8);
	}
}
//—------------------------------------------------------------
// Scrie un sir de lungime dataLen, preluat din memorie incepand cu adresa
//destDataPtr, la capatul cozii de transmitere a datelor
// Functia este blocanta. Va astepta pana se va scrie numarul de intregi
//dorit
void MboxWriteBlocking(int MboxIfBaseAddr, int* srcDataPtr, int dataLen) {
	int i;
	for (i = 0; i < dataLen; i++) {
		while(MboxIsFull(MboxIfBaseAddr))
		{}
		*(volatile u32*)(MboxIfBaseAddr) = *(srcDataPtr + i);
	}
}


int main() {
	char buff_c1[32] = "Initial msg.\n";
	MboxReadBlocking(MBOX_ADDR, (int*)buff_c1, 12/4);
	SendString(buff_c1);

	return 0;

}
