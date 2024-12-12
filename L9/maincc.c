#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>

const int n = 1061;	

void print_hist(int *a, int nr) {
	//a[5] = 71;
	int i, j;

	for (j = 0; j < nr; j++) {
		a[j] = a[j] / 10 + ((a[j] % 10 > 0) && (a[j] / 10 > 0));
	}
	for (i = 0; i < 16; i++) {
		printf("%d ", a[i]);
	}
	printf("\n");
	for (i = 0; i < 10; i++) {
			printf("%d", 10 - i - 1);
			
			for (j = 0; j < 16; j++) {
				if (a[j] > 1 && a[j] >= (10 - i - 1 )) {
					printf(" #");
				} else {
					printf("  ");
				}
			}
		printf("\n");
	}
		printf("+");
		for (j = 0; j < nr; j++) {
			printf(" -");
		}

}

// nr 100%
// hist[i] x%
// x => hist[i] * 100 / nr

void histogram(int *hist, int *arr,int nr) {
	
	for (int i = 0; i < n; i++) {
		hist[arr[i] / 16]++;		 
	}

	hist[rand() % 16] += 1000;
	hist[rand() % 16] += 100;
	hist[rand() % 16] += 100;
	hist[rand() % 16] += 100;

	int s = 0;

	for (int i = 0; i < 16; i++) {
		hist[i] = (hist[i] * 1000) / (n + 400) / 10;
		s += hist[i];
	}
	
}

int *interclasare(int *a, const int nr1, int b[], const int nr2) {
	const int nr3 = nr1 + nr2;
	int *z = (int *)malloc(sizeof(int) * nr3);
	int ia = 0, ib = 0, iz = 0;
	while (ia < nr1 && ib < nr2) {
		if (a[ia] < b[ib]) {
			z[iz] = a[ia];
			ia += 1;
		} else {
			z[iz] = b[ib];
			ib += 1;
		}
		iz += 1;
	}

	while (ia < nr1) {
		z[iz] = a[ia];
		ia += 1;
		iz += 1;
	}

	while(ib < nr2) {
		z[iz] = b[ib];
		ib += 1;
		iz += 1;
	}

	return z;
}


void printArray(int a[], int n) {
	int i;
	for (i = 0; i < n; i++) {
		printf("%d ", a[i]);
	}
	printf("\n");
}
int main()
{
	srand(time(NULL));
	
	int arr[n];

	for (int i = 0; i < n; i++) {
		arr[i] = rand() % 255;
	}
	
	int hist[16] = {0};
	
	histogram(hist, arr, 16);
	for (int i = 0; i < 16; i++) {
		printf("%d ", hist[i]);
	}

	printf("\n");

	print_hist(hist, 16);
	 

	// // int a[] = {1, 4, 6, 8, 11};
	// // int b[5] = {3, 4, 5, 7, 9};
	// // int c[5] = {2, 4, 4, 8, 10};
	// // int d[5] = {3, 4, 5, 6, 7};

	// // int *z = interclasare(interclasare(a, 5, b, 5), 10, interclasare(a, 5, b, 5), 10);
	// // printArray(z, 20);
	// int *x = (int *)malloc(sizeof(int) * 5);

	// int *y = x;

	// y[0] = 2;
	// y[1] = 11;

	// printf("%d", x[]);

	// x = NULL;
	return 0;
}