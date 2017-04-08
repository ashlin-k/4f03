/*
SFWRENG 4F03 PA4
Ashlin Kanawaty
Prakhar Garg
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "ppmFile.h"
#include <string.h>

typedef struct {
  unsigned char r,g,b;
} RGB;

void applyBlurFilter(Image* destImage, Image* srcImage, unsigned int height, unsigned int width, unsigned int windowSize);
void setMean(unsigned char r[], unsigned char g[], unsigned char b[], int size, unsigned char* destImagePixel);

int main(int argc, char** argv)
{
  Image *image;           // original image, only the num of rows needed for each process
  Image *imageSlice;      // the processed image, only the num of rows needed for each process
  Image *testImage;       // the final complete image
  unsigned int width;            // width of image
  unsigned int height;           // height of image
  int max = 255;        // max unsigned char value for a pixel channel in RGB   
  int h0;               // height for process 0; h0 = rows[my_rank]

  //filter is of size mxm
  //input.ppm output.ppm 3 A
  //filter is filter type {M or A} median or mean
  char* infile, *outfile;

  //These are variables for console input
  unsigned int radius;              // blur radius

  int my_rank;
  int p;  
  // p is number of processes; processes are numbered 0 to p-1; p is assigned when you call from the command line with
  // mpirun -np p
  int i, dest, source, offset;

  MPI_Status status;

  infile = argv[2];
  outfile = argv[3];
  radius = (unsigned int)atoi(argv[1]);

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  //I may play around with this number later,
  //I want the distribution of intervals to be gaussian
  // int numIntervals = 100000;
  // SearchInterval *intervals;
  // intervals = (SearchInterval*)malloc(sizeof(SearchInterval)*numIntervals);

  int *rows;    // an array, each index tells each process how many rows it must process
  rows = (int*)malloc(sizeof(int)*p);

  if(my_rank == 0)
  {
    printf("infile: %s\n", infile);
    printf("outfile: %s\n", outfile);
    printf("radius: %u\n", radius);
    image = ImageRead(infile);
    width = image->width;
    height = image->height;
    printf("Image is %ux%u\n", width, height);
  }

  // need to send size of width and height
  MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

  //allocating the number of rows to shoot over to a process
  //extra rows will wrap
  for (i = 0 ;i < p ;i++)
    rows[i] = height/p ;
  for (i = 0; i < height % p ; i++)
    rows[i] ++;
  int tag = 0;

  // allocate memory for the image rows each process gets
  if(my_rank != 0)
  {
    if(my_rank == p - 1)
    {   // last process p-1
      image = ImageCreate( width, (rows[my_rank] + (radius/2)) );
      imageSlice = ImageCreate( width, (rows[my_rank] + (radius/2)) );
    }
    else
    {   // processes 1 to p-2
      image = ImageCreate( width, (rows[my_rank] + (radius-1)) );
      imageSlice = ImageCreate( width, (rows[my_rank] + (radius-1)) );
    }
  }
  else
  {   // process 0
    if(p==1)
    {   // if there is only one process
      h0 = rows[my_rank];
    }
    else
    {    // if there are multiple processes
      h0 = rows[my_rank] + radius/2;
    }

    testImage = ImageCreate( width, height );
    imageSlice = ImageCreate( width, h0 );
    memcpy( imageSlice->data, image->data, (h0 * width * sizeof(unsigned char) * 3) );
  }

  // send the original image to each process
  if(my_rank == 0)
  {
      int offset = 0;
      for(dest = 1; dest < p; dest++)
      {
        offset += rows[dest - 1];
        //buffer, count, filler...
        //The first and last process need the duplicate pixels from previous/last slice
        if(dest == p - 1)
        {   // last process, p-1
          MPI_Send(&image->data[(offset - radius/2) * width],
            (rows[dest] + (radius/2)) * width * sizeof(unsigned char) * 3,
            MPI_BYTE, dest, tag, MPI_COMM_WORLD);
        }
        else
        {     // other processes 1 to p-2
          MPI_Send(&image->data[(offset - radius/2) * width],
            (rows[dest] + (radius-1)) * width * sizeof(unsigned char) * 3,
            MPI_BYTE, dest, tag, MPI_COMM_WORLD);
        }
      }
  }

  // receive original image
  else
  {
    if(my_rank == p - 1)
    {
      MPI_Recv(image->data,
        (rows[my_rank] + (radius/2)) * width * sizeof(unsigned char) * 3,
        MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
    }
    else
    {
      MPI_Recv(image->data,
        (rows[my_rank] + (radius-1)) * width * sizeof(unsigned char)  * 3,
        MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
    }
  }

  // process the image
  printf("Computing filter on process: %d\n", my_rank);
  if(my_rank != 0)
  {
    if(my_rank == p - 1)
    {
      applyBlurFilter(imageSlice, image, rows[my_rank] + radius/2, width, radius);
    }
    else
    {
      applyBlurFilter(imageSlice, image, rows[my_rank] + (radius-1), width, radius);
    }
  }
  else
  {
    // for process 0, imageSlice is just a copy of image
    //printf("processing %d  with length %lu from %d\n",h0 * width, rows[my_rank] * width * sizeof(RGB), 0);
    applyBlurFilter(testImage, imageSlice, h0, width, radius);
  }

  // send the data back to process 0
  if(my_rank != 0)
  {
        MPI_Send(&imageSlice->data[(radius/2) * width],
          rows[my_rank] * width * sizeof(unsigned char) * 3,
          MPI_BYTE, 0, tag, MPI_COMM_WORLD);
        printf("sending image\n" );
  }
  else
  {
    offset = 0;
    for(source = 1; source < p; source++)
    {
        offset += rows[source - 1];
        MPI_Recv(&testImage->data[(offset) * width],
          rows[source] * width * sizeof(unsigned char) * 3,
          MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
    }
  }

  // write the final image
  if(my_rank ==0)
  {
    printf("Writing image...\n");
    ImageWrite(testImage, outfile);
    free(testImage);
  }
  printf("Freeing memory!\n");

  free(imageSlice);
  free(rows);
  free(image);

  MPI_Finalize();

  return 0;
}

//iterate over every pixel
//for every pixel, take the window of pixels and average the R, G, B vals
void applyBlurFilter(Image* destImage, Image* srcImage, unsigned int height, unsigned int width, unsigned int windowSize)
{
  unsigned int i,j= 0;
  unsigned int row, col, size;
  printf("width: %u\n", width);
  printf("height: %u\n", height);
  printf("window size: %u\n", windowSize);

  for (i=0; i < height ;  i ++)
  {
    for(j=0; j  < width ; j ++)
    {
      unsigned char red[windowSize*windowSize];
      unsigned char green[windowSize*windowSize];
      unsigned char blue[windowSize*windowSize];

      size = 0;
      for(row = (i - windowSize/2) * 3; row < (i + windowSize/2) * 3 ; row+=3)
      {
        for(col = (j - windowSize/2) * 3; col < (j + windowSize/2) * 3; col+=3)
        {
          // printf("i=%d, j=%d, size=%d, it=%d\n", i, j, size, row * width + col);
          red[size] = srcImage->data[row * width + col];
          green[size] = srcImage->data[row * width + col + 1];
          blue[size] = srcImage->data[row * width + col + 2];
          size+=1;
        }
      }
      
      setMean(red, green, blue, size, &destImage->data[i*width + j]);    

    }
  }
  printf("Completed blurring.\n");
}

void setMean(unsigned char r[], unsigned char g[], unsigned char b[], int size, unsigned char* destImagePixel)
{
  unsigned char sumR = 0;
  unsigned char sumG = 0;
  unsigned char sumB = 0;
  int j;
  for(j = 0; j < size; j++)
  {
     sumR += r[j];
     sumG += g[j];
     sumB += b[j];
  }

  if(size !=0)
  {
    *destImagePixel = sumR / size;
    *(destImagePixel + 1) = sumG / size;
    *(destImagePixel + 2) = sumB / size;
  }
}
