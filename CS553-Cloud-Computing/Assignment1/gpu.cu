/*Ron Pyka
  CS 553
  Assignment 1
  GPU Benchmark */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>

#define BLOCK_SIZE 16

/* Arrays */
volatile float A[1], B[1000], C[1000000], D[10000][10000];
volatile int E[10000][10000];

/* Initialize D and E*/
void initialize_inputs() {
  int row, col;

  printf("\nInitializing...\n");
  for (col = 0; col < 10000; col++) {
    for (row = 0; row < 10000; row++) {
      D[row][col] = (float)rand() / 32768.0;
      E[row][col] = (int)rand() / 32768;
    }
  }

}

__global__ void gpuFlopTest(float *X){
    int row = blockIdx.y*blockDim.y + threadIdx.y;
    int col = blockIdx.x*blockDim.x + threadIdx.x;

    X[row*10000 + col] = X [row*10000 + col] * 2.2;
}

__global__ void gpuIopTest(int *X){
    int row = blockIdx.y*blockDim.y + threadIdx.y;
    int col = blockIdx.x*blockDim.x + threadIdx.x;

    X[row*10000 + col] = X [row*10000 + col] + 2;
}

int main(void)
{
    /* Timing variables */
    struct timeval etstart, etstop;  /* Elapsed times using gettimeofday() */
    struct timezone tzdummy;
    unsigned long long usecstart, usecstop;

    int sizeA = 1*sizeof(float);
    int sizeB = 1000*sizeof(float);
    int sizeC = 1000000*sizeof(float);
    int sizeD = 10000*10000*sizeof(float);
    int sizeE = 10000*10000*sizeof(int);
	float *X;
	int *Y;

	int K = ceil((float)10000/((float)BLOCK_SIZE));

    dim3 threadBlock(BLOCK_SIZE, BLOCK_SIZE);
    dim3 grid(K, K);

    printf("Testing memory speed\n\n");
	cudaMalloc(&X, sizeA);

    printf("4B write.\n");
	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);
	for(int i=0;i<500000;i++){
    	cudaMemcpy(X,(void **)A,sizeA,cudaMemcpyHostToDevice);
	}
	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

    printf("4B read.\n");
	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);
	for(int i=0;i<500000;i++){
	    cudaMemcpy((void **)A,X,sizeA,cudaMemcpyDeviceToHost);
	}
	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

	cudaFree(&X);

	cudaMalloc(&X, sizeB);

    printf("4KB write.\n");
	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);
	for(int i=0;i<500000;i++){
    	cudaMemcpy(X,(void **)B,sizeB,cudaMemcpyHostToDevice);
	}
	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

    printf("4KB read.\n");
	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);
	for(int i=0;i<500000;i++){
	    cudaMemcpy((void **)B,X,sizeB,cudaMemcpyDeviceToHost);
	}
	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

    cudaFree(&X);

	cudaMalloc(&X, sizeC);

    printf("4MB write.\n");
	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);
	for(int i=0;i<50000;i++){
    	cudaMemcpy(X,(void **)C,sizeC,cudaMemcpyHostToDevice);
	}
	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

    printf("4MB read.\n");
	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);
	for(int i=0;i<50000;i++){
	    cudaMemcpy((void **)C,X,sizeC,cudaMemcpyDeviceToHost);
	}
	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

	cudaFree(&X);

	printf("\n\nTesting Flops and Iops\n\n");

	initialize_inputs();

	printf("Flops.\n");

	cudaMalloc(&X, sizeD);
    cudaMemcpy(X,(void **)D,sizeD,cudaMemcpyHostToDevice);

	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);

    gpuFlopTest<<<grid, threadBlock>>>(X);

	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

	cudaFree(&X);

	printf("Iops.\n");

	cudaMalloc(&Y, sizeE);
    cudaMemcpy(Y,(void **)E,sizeE,cudaMemcpyHostToDevice);

	printf("\nStarting clock.\n");
    gettimeofday(&etstart, &tzdummy);

    gpuIopTest<<<grid, threadBlock>>>(Y);

	gettimeofday(&etstop, &tzdummy);
	usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
    usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;

    printf("\nElapsed time = %g ms.\n",
	 (float)(usecstop - usecstart)/(float)1000);

	cudaFree(&Y);
}
