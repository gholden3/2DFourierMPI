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
 // int runningSum = 0;
  double angle;
  double sumReal;
  double sumImag;
  // Implement a simple 1-d DFT using the double summation equation
  //   // given in the assignment handout.  h is the time-domain input
  //     // data, w is the width (N), and H is the output array.
  for (int n = 0; n < N; ++n)
    { // for each element
    sumReal = 0;
    sumImag = 0;
    for (int k=0; k<N;k++) //sum accross all the elements
      {
     // hOfk = h[k];
      angle = 2 * M_PI * n * k / N;
      sumReal +=  h[k].real * cos(angle) + h[k].imag * sin(angle);
      sumImag +=  -h[k].real * sin(angle) + h[k].imag * cos(angle);
     // runningSum = runningSum + (M_E)^(((-sqrt(-1))*2*m_PI)/N)
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
  Complex* recieveArr;

  ofstream ofs("outFileG.txt"); //output file for debugging and testing
  ofs << "Hello world!" << endl;
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
  InputImage image(inputFN);  // Create the helper object for reading the image
  w = image.GetWidth();
  h = image.GetHeight();

  // 2) Use MPI to find how many CPUs in total, and which one this process is
  MPI_Comm_size(MPI_COMM_WORLD, &numCPUs); //how many CPUs? 
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); //which CPU am I?
  if(rank==1)
  ofs  << "Number of tasks= " << numCPUs << ".  My rank= " << rank<< endl;

  // 3) Allocate an array of Complex object of sufficient size to
  //     hold the 2d DFT results (size is width * height)
  OneDResultArr = new Complex[w*h]; //output array. 
 
  // 4) Obtain a pointer to the Complex 1d array of input data
  myData = image.GetImageData();

  // 5) Do the individual 1D transforms on the rows assigned to your CPU
  rowsPerCPU = h / numCPUs;
  //if(rank==1)
  //ofs<< "rowsPerCPU: " << rowsPerCPU << endl;
  int rowStart = rowsPerCPU * rank; // the row I need to start my 1D transforms on
  int rowEnd = rowStart + rowsPerCPU; // the row I need to end on
  //if(rank==1){
  //ofs << "I need to start my transformations on row: " << rowStart 
  //    << ". I need to end on row: " << rowEnd << endl;
  //}
  for(rowStart; rowStart<rowEnd;rowStart++)
    { // for each row of work 
    Complex* hPtr  = myData + rowStart *w; //where to start reading input data
    Complex* HPtr = OneDResultArr + rowStart *w; //where to start writing output data
    Transform1D(hPtr, w, HPtr); //transform that row and write into resultArray
    } 
  if(rank != 0) // send my transform back to CPU 0 
    {
    //if(rank==1)
    //ofs << "i'm rank " <<  rank << "sending back to rank 0." << endl;
    rc = MPI_Send(OneDResultArr, w*h, MPI_COMPLEX, 0, 0, MPI_COMM_WORLD);
    }
  if(rank == 0)
    { //my work is already in correct place in 1DResultArr
      //block recieve all the other ones
      for(int i = 1; i<numCPUs; i++)
      { int recSize = w*h;
        recieveArr = new Complex[recSize];
        MPI_Status status;
        rc = MPI_Recv(recieveArr, recSize, MPI_COMPLEX, i, 0, MPI_COMM_WORLD, &status);
        //put that array into the correct place in 1DResultArr
        rowStart = rowsPerCPU * i; // the row that computations started on
        rowEnd = rowStart + rowsPerCPU;
        for(rowStart; rowStart<rowEnd;rowStart++)
          { // for each row of work 
            for(int j=0;j<w;j++) //for each column
              { 
              Complex* hPtr  = recieveArr + (rowStart * w); //row to start reading
              Complex* HPtr = OneDResultArr + (rowStart * w) ; //row to start writing
              HPtr[j].real = hPtr[j].real;
              HPtr[j].imag = hPtr[j].imag;
              }
          } 
      }
      image.SaveImageData("outData.txt", OneDResultArr, w, h);
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
  

 
