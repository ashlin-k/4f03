//File: hello.cu

#include <stdio.h>
#include <cuda.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ppmFile.h"

//GPU dimensions
int blockX =  5;
int blockY =  5;
int gridX =  5;
int gridY =  5;
int gridZ =  4;





__global__ void vadd(int *redin, int *greenin, int *bluein,
					int *redout, int *greenout, int *blueout,
					int radius) {
					
	int *pixin, *pixout;
//calculate thread ID
	int myID = blockIdx.x * gridDim.z * gridDim.y * blockDim.x * blockDim.y
				+blockIdx.y * gridDim.z * blockDim.x * blockDim.y
				+blockIdx.z * blockDim.x * blockDim.y
				+threadIdx.x * blockDim.y
				+threadIdx.y;
	
	
//calculate AVG
	//find col and row of px
	
		int pixID = blockIdx.y * gridDim.x * blockIdx.x;
		int colMin = blockIdx.x - radius;
		int colMax = blockIdx.x + radius;
		int rowMin = blockIdx.y - radius;
		int rowMax = blockIdx.y + radius;
		
		if (colMin <= 0)
			colMin = 0;
		if (colMax >= gridDim.x)
			colMax = gridDim.x-1;
		if (rowMin <= 0)
			rowMin = 0;
		if (rowMax >= gridDim.y)
			rowMax = gridDim.y-1;

			
		if (threadIdx.x == 0)
			pixin = redin;
		if (threadIdx.x == 1)
			pixin = greenin;
		if (threadIdx.x == 2)
			pixin = bluein;
		//sum
		for(int c = colMin; c <= colMax; c++){
			for(int r = rowMin; r <= rowMax; r++){
				pixout[pixID] += pixin[r * gridDim.x + c];
			}
		}
		
		//avg
		pixout[pixID] = (pixout[pixID] / (colMax - colMin + 1) * (rowMax - rowMin + 1)) % 255;
		
		//check for error
		if(pixout[pixID] < 0)
			pixout[pixID] = 0;

		if (threadIdx.x == 0)
			redout = pixout;
		if (threadIdx.x == 1)
			greenout = pixout;
		if (threadIdx.x == 2)
			blueout = pixout;
			
}


int main(int argc, char** argv) {
printf("test");
//begin timer
clock_t startTime = clock();

//init variables
	Image *image;     		// original image, only the num of rows needed for each process
	Image *finalImage; 		// the final complete image
	int width;				// width of image
	int height;				// height of image
	char* infile, *outfile;	// input.ppm output.ppm 3 A
	unsigned long radius;	// blur radius
	int *red, *blue, *green,
		*redin, *greenin, *bluein,
		*redout, *greenout, *blueout,
		*pixelsToCompute, *pixelsToComputeGPU;

//assign variables
  infile = argv[2];
  outfile = argv[3];
  radius = (unsigned int)atol(argv[1]);
  
//display arguments
    printf("Infile: %s\n", infile);
    printf("Outfile: %s\n", outfile);
    printf("Radius: %lu\n", radius);

//read input image
    image = ImageRead(infile);
    width = image->width;
    height = image->height;
    printf("Image is %ix%i\n", width, height);

//set up image arrays
    red = new int[width*height];
    green = new int[width*height];
    blue = new int[width*height];
	


//assign values to array	
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++){
			long pos = row * width + col;			
			red[pos] = ImageGetPixel(image, col, row, 0);
			green[pos] = ImageGetPixel(image, col, row, 1);
			blue[pos] = ImageGetPixel(image, col, row, 2);
		}
	}

//allocate memory for CUDA	
	cudaMalloc ((void **) &redin, sizeof (int) * width*height);
	cudaMalloc ((void **) &greenin, sizeof (int) * width*height);
	cudaMalloc ((void **) &bluein, sizeof (int) * width*height);
	cudaMalloc ((void **) &redout, sizeof (int) * width*height);
	cudaMalloc ((void **) &greenout, sizeof (int) * width*height);
	cudaMalloc ((void **) &blueout, sizeof (int) * width*height);

	
//copy relavent arrays to memory
	cudaMemcpy (redin, red, sizeof (int) * width*height, cudaMemcpyHostToDevice);
	cudaMemcpy (greenin, green, sizeof (int) * width*height, cudaMemcpyHostToDevice);
	cudaMemcpy (bluein, blue, sizeof (int) * width*height, cudaMemcpyHostToDevice);

	
//Start GPU
	dim3 block (3);
	dim3 grid (width, height);
	vadd <<<grid,block>>> (redin, greenin, bluein,
							redout, greenout, blueout,
							radius);
							
//stop GPU
	cudaDeviceSynchronize();
	cudaGetLastError();

//get data
	cudaMemcpy (red, redout, sizeof (int) * width*height, cudaMemcpyDeviceToHost);
	cudaMemcpy (green, greenout, sizeof (int) * width*height, cudaMemcpyDeviceToHost);
	cudaMemcpy (blue, blueout, sizeof (int) * width*height, cudaMemcpyDeviceToHost);

//construct new image
	finalImage = ImageCreate(width, height);
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++){
			long pos = row * width + col;			
			ImageSetPixel(finalImage, col, row, 0, red[pos]);
			ImageSetPixel(finalImage, col, row, 1, green[pos]);
			ImageSetPixel(finalImage, col, row, 2, blue[pos]);
		}
	}
	ImageWrite(finalImage, outfile);
	
//cleanup
	cudaFree ((void *) redin);
	cudaFree ((void *) greenin);
	cudaFree ((void *) bluein);
	cudaFree ((void *) redout);
	cudaFree ((void *) greenout);
	cudaFree ((void *) blueout);
	if (image != NULL)free(image);
	if (finalImage != NULL)free(finalImage);
	delete []red;
	delete []green;
	delete []blue;
	cudaDeviceReset ();
	

//end timer
clock_t stopTime = clock();
double time_spent = (double)(stopTime - startTime) / CLOCKS_PER_SEC;
printf("execution time: %f\n",time_spent);
	
return 0;
}




