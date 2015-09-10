// Distributed two-dimensional Discrete FFT transform
// YOUR NAME HERE
// ECE8893 Project 1


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <signal.h>
#include <math.h>
#include <mpi.h>
#include <stdlib.h>
#include <cmath>

#include "Complex.h"
#include "InputImage.h"

#define MSG_SIZE 1000
#define _USE_MATH_DEFINES

Complex  buf[MSG_SIZE]; //message buffer to send and recieve data from other CPUs

using namespace std;



void Transform1D(Complex* h, int N, Complex* H)
{ 
  double angle;
  double sumReal;
  double sumImag;
  /* Implement a simple 1-d DFT using the double summation equation
  given in the assignment handout.  h is the time-domain input
  data, w is the width (N), and H is the output array. */
  for (int n = 0; n < N; ++n)
    { // for each element
    sumReal = 0;
    sumImag = 0;
    for (int k=0; k<N;k++) //sum accross all the elements
      {
      angle = 2 * M_PI * n * k / N;
      sumReal +=  h[k].real * cos(angle) + h[k].imag * sin(angle);
      sumImag +=  -h[k].real * sin(angle) + h[k].imag * cos(angle);
      }
    H[n].real = sumReal;
    H[n].imag = sumImag;
    }
}

void Transform2D(const char* inputFN) 
{ 
  int w, h;
  int numCPUs, rank, rc, rowsPerCPU;

  Complex* myData;  
  Complex* hPtr;
  Complex* HPtr;
  Complex* OneDResultArr;
  Complex* TwoDResultArr;
  Complex* sendPtr;
  Complex* recvPtr;
  Complex* columnSendArr; //filled by CPU0 to send cols to other CPUs
  Complex* columnRecvArr; //filled by 1DFourier transform of cols
  MPI_Status status;
  //ofstream ofs("outFileG.txt"); //output file for debugging and testing
 // Do the 2D transform here.
  // 6) Send the resultant transformed values to the appropriate
  //    other processors for the next phase.
  // 6a) To send and receive columns, you might need a separate
  //     Complex array of the correct size.
  // 7) Receive messages from other processes to collect your columns
  // 8) When all columns received, do the 1D transforms on the columns
  // 9) Send final answers to CPU 0 (unless you are CPU 0)
  //   9a) If you are CPU 0, collect all values from other processors
  //       and print out with SaveImageData().

  // 1) Use the InputImage object to read in the Tower.txt file and
  //    find the width/height of the input image.
  // Create the helper object for reading the image
  InputImage image(inputFN); 
  w = image.GetWidth();
  h = image.GetHeight();

  // 2) Use MPI to find how many CPUs in total, and which one this process is
  MPI_Comm_size(MPI_COMM_WORLD, &numCPUs); //how many CPUs? 
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); //which CPU am I?
  /*if(rank==1)
  ofs  << "Number of tasks= " << numCPUs << ".  My rank= " << rank<< endl;*/

  // 3) Allocate an array of Complex object of sufficient size to
  //     hold the 2d DFT results (size is width * height)
  OneDResultArr = new Complex[w*h]; //output array. 
  TwoDResultArr = new Complex[w*h];
 
  // 4) Obtain a pointer to the Complex 1d array of input data
  myData = image.GetImageData();

  // 5) Do the individual 1D transforms on the rows assigned to your CPU
  rowsPerCPU = h / numCPUs;
  int colsPerCPU = w / numCPUs;
  columnRecvArr = new Complex[colsPerCPU*h];
  int rowStart = rowsPerCPU * rank; // the row I need to start my 1D transforms on
  int rowEnd = rowStart + rowsPerCPU; // the row I need to end on
  columnSendArr = new Complex[colsPerCPU*h];
  int colMsgSize = colsPerCPU*h*sizeof(Complex);
  for(rowStart; rowStart<rowEnd;rowStart++)
    { // for each row of work 
    hPtr  = myData + rowStart *w; //where to start reading input data
    HPtr = OneDResultArr + rowStart *w; //where to start writing output data
    Transform1D(hPtr, w, HPtr); //transform that row and write into resultArray
    }
     
  int msgSize = rowsPerCPU*w * sizeof(Complex);
  
  if(rank != 0) // send my transform back to CPU 0 
    {//sendPtr points to where my rows start
    sendPtr = OneDResultArr + rowsPerCPU*rank*w;
    //each CPU sends its work back to CPU 0
    rc = MPI_Send(sendPtr, msgSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    // now needs to receive the row array, perform transforms, and send row array back
       
    rc =  MPI_Recv(columnSendArr,colMsgSize,MPI_CHAR,MPI_ANY_SOURCE,0,MPI_COMM_WORLD, &status);
    hPtr = columnSendArr;
    HPtr = columnRecvArr;

    for(int r =0; r<colsPerCPU;r++)//for each row in column array
      {
      Transform1D(hPtr, w, HPtr);
      hPtr += w;
      HPtr += w;
      }
    //now send columnRecvArr back to CPU0
    rc = MPI_Send(columnRecvArr,colMsgSize,MPI_CHAR,0,0,MPI_COMM_WORLD);
    }
  if(rank == 0)
    { //my work is already in correct place in 1DResultArr
      //block recieve all the other ones
      for(int i = 1; i<numCPUs; i++)
        { 
        MPI_Status status;
        recvPtr = OneDResultArr + rowsPerCPU*i*w;
        rc = MPI_Recv(recvPtr, msgSize, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
        }
      //CPU0 now needs to send each column to the other CPUs
    

      for(int i=1;i<numCPUs; i++)//for each cpu
      {//fill the send array and send it all its columns
        int startCol = i*colsPerCPU;
        int endCol = startCol+colsPerCPU;
        for( int j= startCol;j<endCol;j++)//for each column I need to send
          {
           //for each row I need to send
           for(int r =0;r<h;r++)
             {
             //put data into send array
             columnSendArr[(j-startCol)*w+r] = OneDResultArr[r*w +j];
             }
          }
        //when columnSendArr is full, send it to the right CPU
        rc =  MPI_Send(columnSendArr,colMsgSize,MPI_CHAR,i,0,MPI_COMM_WORLD);
      }

      
      //here CPU0 does its work on columns on OneDResultArr
      for(int j = 0;j<colsPerCPU;j++)//for each column 
        {
        //for each row
        for(int r =0;r<h;r++)
          {
          //put data into array (flipping columns into rows)
          columnSendArr[j*w+r] = OneDResultArr[r*w+j];
          }
        }
      
      hPtr = columnSendArr;
      HPtr = columnRecvArr;
      for(int r =0; r<colsPerCPU;r++)//for each row in column array
        {
        Transform1D(hPtr, w, HPtr);
        hPtr += w;
        HPtr += w; 
        }
      //write results into columns of TwoDResultArray
      for(int i=0;i<colsPerCPU;i++)//for each row 
        { 
        //for each column
        for(int j =0;j<w;j++)
          {
          //put data into array (flipping back into columns)
          TwoDResultArr[j*w+i] = columnRecvArr[i*w+j]; 
          }
        }

       for(int i = 1; i<numCPUs; i++)//for each CPU, receive columns
        {
        MPI_Status status;
        rc = MPI_Recv(columnRecvArr, colMsgSize, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
        for(int k=0;k<colsPerCPU;k++)//for each row 
          {
          //for each column
          for(int j =0;j<w;j++)
            {
            //put data into array (flipping back into columns)
            int flatIndex = (j*w)+k+(i*colsPerCPU);
            TwoDResultArr[flatIndex] = columnRecvArr[k*w+j];
            }
          }
        }

      image.SaveImageData("MyAfter2d.txt", TwoDResultArr, w, h);
    }
}


int main(int argc, char** argv)
{ 
  int rc ; 
  string fn("Tower.txt"); // default file name
  if (argc > 1) fn = string(argv[1]);  // if name specified on cmd line
  // MPI initialization here
  rc = MPI_Init(&argc, &argv);
  if(rc!=MPI_SUCCESS){
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  Transform2D(fn.c_str()); // Perform the transform.
  // Finalize MPI here
  MPI_Finalize(); 
 
}  
