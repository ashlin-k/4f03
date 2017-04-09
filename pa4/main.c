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

int numProcesses; 

void applyBlurFilter(int myRank, Image* destImage, Image* srcImage, unsigned long height, unsigned long width, unsigned long windowSize);
void setMean(int myRank, Image* image, unsigned char r[], unsigned char g[], unsigned char b[], unsigned long size, unsigned long x, unsigned long y);

int main(int argc, char** argv)
{
  Image *image;           // original image, only the num of rows needed for each process
  Image *imageSlice;      // the processed image, only the num of rows needed for each process
  Image *testImage;       // the final complete image
  unsigned long width;            // width of image
  unsigned long height;           // height of image
  unsigned long h0;               // height for process 0; h0 = rows[my_rank]

  //filter is of size mxm
  //input.ppm output.ppm 3 A
  //filter is filter type {M or A} median or mean
  char* infile, *outfile;

  //These are variables for console input
  unsigned long radius;              // blur radius
  unsigned long filterSideLength = 0;

  int my_rank; 
  // numProcesses is number of processes; processes are numbered 0 to numProcesses-1; numProcesses is assigned when you call from the command line with
  // mpirun -np numProcesses
  unsigned long i, dest, source, offset;

  MPI_Status status;

  infile = argv[2];
  outfile = argv[3];
  radius = (unsigned int)atol(argv[1]);
  filterSideLength = 2* radius + 1;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  // MPI timer
  // **** START TIMER **** //
  double local_start, local_stop, local_elapsed, elapsed;
  MPI_Barrier(MPI_COMM_WORLD);
  local_start = MPI_Wtime();

  unsigned long *rows;    // an array, each index tells each process how many rows it must process
  rows = (unsigned long*)malloc(sizeof(unsigned long)*numProcesses);

  if(my_rank == 0)
  {
    printf("Infile: %s\n", infile);
    printf("Outfile: %s\n", outfile);
    printf("Radius: %lu\n", radius);
    image = ImageRead(infile);
    width = image->width;
    height = image->height;
    printf("Image is %lux%lu\n", width, height);
  }

  // need to send size of width and height
  MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

  //allocating the number of rows to shoot over to a process
  //extra rows will wrap
  for (i = 0 ;i < numProcesses ;i++)
    rows[i] = height/numProcesses ;
  for (i = 0; i < height % numProcesses ; i++)
    rows[i] ++;
  int tag = 0;

  // allocate memory for the image rows each process gets
  if(my_rank != 0)
  {
    if(my_rank == numProcesses - 1)
    {   // last process numProcesses-1
      image = ImageCreate( width, (rows[my_rank] + (filterSideLength/2)) );
      imageSlice = ImageCreate( width, (rows[my_rank] + (filterSideLength/2)) );
    }
    else
    {   // processes 1 to numProcesses-2
      image = ImageCreate( width, (rows[my_rank] + (filterSideLength-1)) );
      imageSlice = ImageCreate( width, (rows[my_rank] + (filterSideLength-1)) );
    }
  }
  else
  {   // process 0
    if(numProcesses==1)
    {   // if there is only one process
      h0 = rows[my_rank];
    }
    else
    {    // if there are multiple processes
      h0 = rows[my_rank] + filterSideLength/2;
    }

    testImage = ImageCreate( width, height );
    imageSlice = ImageCreate( width, h0 );
    memcpy( imageSlice->data, image->data, (h0 * width * sizeof(unsigned char) * 3) );
    // printf("Reading P0: offset (in rows)=0, starting_row=0, size=%lu\n", h0 );
  }

  // send the original image to each process
  if(my_rank == 0)
  {
      unsigned int offset = 0;
      for(dest = 1; dest < numProcesses; dest++)
      {
        offset += rows[dest - 1];
        //buffer, count, filler...
        //The first and last process need the duplicate pixels from previous/last slice
        if(dest == numProcesses - 1)
        {   // last process, numProcesses-1
          MPI_Send(&image->data[(offset - filterSideLength/2) * width * 3],
            (rows[dest] + (filterSideLength/2)) * width * sizeof(unsigned char) * 3,
            MPI_BYTE, dest, tag, MPI_COMM_WORLD);
          // printf("Reading P%lu: offset (in rows)=%u, starting_row=%lu, size=%lu\n", dest, offset, (offset - filterSideLength/2), 
          //   (rows[dest] + (filterSideLength/2)) );
        }
        else
        {     // other processes 1 to numProcesses-2
          MPI_Send(&image->data[(offset - filterSideLength/2) * width * 3],
            (rows[dest] + (filterSideLength-1)) * width * sizeof(unsigned char) * 3,
            MPI_BYTE, dest, tag, MPI_COMM_WORLD);
          // printf("Reading P%lu: offset (in rows)=%u, starting_row=%lu, size=%lu\n", dest, offset, (offset - filterSideLength/2), 
          //   (rows[dest] + (filterSideLength-1)) );
        }
      }
  }

  // receive original image
  else
  {
    if(my_rank == numProcesses - 1)
    {
      MPI_Recv(image->data,
        (rows[my_rank] + (filterSideLength/2)) * width * sizeof(unsigned char) * 3,
        MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
    }
    else
    {
      MPI_Recv(image->data,
        (rows[my_rank] + (filterSideLength-1)) * width * sizeof(unsigned char)  * 3,
        MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
    }
  }

  // process the image
  // printf("Computing filter on process: %d\n", my_rank);
  if(my_rank != 0)
  {
    if(my_rank == numProcesses - 1)
    {
      applyBlurFilter(my_rank, imageSlice, image, rows[my_rank] + filterSideLength/2, width, filterSideLength);
    }
    else
    {
      applyBlurFilter(my_rank, imageSlice, image, rows[my_rank] + (filterSideLength-1), width, filterSideLength);
    }
  }
  else
  {
    // for process 0, imageSlice is just a copy of image
    //printf("processing %d  with length %lu from %d\n",h0 * width, rows[my_rank] * width * sizeof(RGB), 0);
    applyBlurFilter(my_rank, testImage, imageSlice, h0, width, filterSideLength);
    // printf("Writing P0: offset (in rows)=0, size=%lu\n", rows[0] );
  }

  // send the data back to process 0
  if(my_rank != 0)
  {
        MPI_Send(&imageSlice->data[(filterSideLength/2) * width * 3],
          rows[my_rank] * width * sizeof(unsigned char) * 3,
          MPI_BYTE, 0, tag, MPI_COMM_WORLD);
        // printf("sending image\n" );
  }
  else
  {
    offset = 0;
    for(source = 1; source < numProcesses; source++)
    {
        offset += rows[source - 1];
        MPI_Recv(&testImage->data[offset * width * 3],
          rows[source] * width * sizeof(unsigned char) * 3,
          MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
        // printf("Writing P%lu: offset (in rows)=%lu, size=%lu\n", source, offset, rows[source] );
    }
    // printf("received image\n" );
  }

  // write the final image
  if(my_rank ==0)
  {
    ImageWrite(testImage, outfile);   
    free(testImage);
  }
  

  // if (testImage != NULL) free(testImage);
  if (imageSlice != NULL) free(imageSlice);
  if (rows != NULL) free(rows);
  if (image != NULL)free(image);

  // **** STOP TIMER **** //
  local_stop = MPI_Wtime();
  local_elapsed = local_stop - local_start;
  MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  if (my_rank == 0)
    printf("Elapsed time is %.2f seconds.\n", elapsed);

  MPI_Finalize();

  return 0;
}

//iterate over every pixel
//for every pixel, take the window of pixels and average the R, G, B vals
void applyBlurFilter(int myRank, Image* destImage, Image* srcImage, unsigned long height, unsigned long width, unsigned long windowSize)
{
  unsigned long i,j= 0;
  unsigned long row, col, size;
  // printf("width: %lu\n", width);
  // printf("height: %lu\n", height);
  // printf("window size: %lu\n", windowSize);

  unsigned long radius = (windowSize - 1) / 2;
  unsigned long startingRow, endingRow, startFilterRow, startFilterCol, endFilterRow, endFilterCol;

  if (myRank == 0) 
  {
    startingRow = 0;
    endingRow = height - radius;
  }
  else if (myRank == numProcesses - 1) 
  {
    startingRow = radius;
    endingRow = height;
  }
  else 
  {
    startingRow = radius;
    endingRow = height - radius;
  }

  for (i=startingRow; i < endingRow;  i ++)    // row
  {
    for(j=0; j  < width ; j ++)       // column
    {
      unsigned char red[windowSize*windowSize];
      unsigned char green[windowSize*windowSize];
      unsigned char blue[windowSize*windowSize];
      
      size = 0;

      // startFilterRow = i - windowSize/2;
      // startFilterCol = j - windowSize/2;

      if (windowSize/2 > i) startFilterRow = 0;
      else startFilterRow = i - windowSize/2;
      if (windowSize/2 > j) startFilterCol = 0;
      else startFilterCol = j - windowSize/2;
      if ((i + windowSize/2) > height) endFilterRow = height;
      else endFilterRow = i + windowSize/2;
      if ((j + windowSize/2) > width) endFilterCol = width;
      else endFilterCol = j + windowSize/2;

      for(row = startFilterRow; row < endFilterRow; row++)
      {
        for(col = startFilterCol; col < endFilterCol; col++)
        {
          red[size] = ImageGetPixel(srcImage, col, row, 0);
          green[size] = ImageGetPixel(srcImage, col, row, 1);
          blue[size] = ImageGetPixel(srcImage, col, row, 2);
          size++;       
        }
      }

      setMean(myRank, destImage, red, green, blue, size, j, i);          
    }
  }
  // printf("Completed blurring.\n");
}

void setMean(int myRank, Image* destImage, unsigned char r[], unsigned char g[], unsigned char b[], unsigned long size, unsigned long x, unsigned long y)
{
  unsigned long sumR = 0;
  unsigned long sumG = 0;
  unsigned long sumB = 0;
  unsigned char avgR = 0;
  unsigned char avgG = 0;
  unsigned char avgB = 0;

  int j;
  for(j = 0; j < size; j++)
  {
     sumR += r[j];
     sumG += g[j];
     sumB += b[j];
  }  

  if(size !=0)
  {
    if (sumR/size > 255)
    {
      avgR = 255;
    }
    else
    {
      avgR = (unsigned char) (sumR / size);
    }
    if (sumR/size > 255)
    {
      avgG = 255;
    }
    else
    {
      avgG = (unsigned char) (sumG / size);
    }
    if (sumR/size > 255)
    {
      avgB = 255;
    }
    else
    {
      avgB = (unsigned char) (sumB / size);
    }

    // if (myRank == 1)
    // {
    //   avgR = 255;
    //   avgG = 0;
    //   avgB = 0;
    // }
    // else if (myRank == 2)
    // {
    //   avgR = 0;
    //   avgG = 255;
    //   avgB = 0;
    // }
    // else if (myRank == 3)
    // {
    //   avgR = 0;
    //   avgG = 0;
    //   avgB = 255;
    // }

    ImageSetPixel(destImage, x, y, 0, avgR);
    ImageSetPixel(destImage, x, y, 1, avgG);
    ImageSetPixel(destImage, x, y, 2, avgB);

  }
  // else printf("size is 0 at x,y = %u,%u\n", x, y);
}
