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

#include "Complex.h"
#include "InputImage.h"

#define MSG_SIZE 1000

char buf[MSG_SIZE]; //message buffer to send and recieve data from other CPUs

using namespace std;


void Transform2D(const char* inputFN) 
{ 

  int w, h;
  int numtasks, rank;

  ofstream ofs("outFileG.txt"); //output file for debugging and testing
  ofs << "Hello world!" << endl;
  Complex* myData;
 // Do the 2D transform here.
  // 3) Allocate an array of Complex object of sufficient size to
  //    hold the 2d DFT results (size is width * height)
  // 4) Obtain a pointer to the Complex 1d array of input data
  // 5) Do the individual 1D transforms on the rows assigned to your CPU
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
  myData = image.GetImageData();

  // 2) Use MPI to find how many CPUs in total, and which one this process is
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks); //how many CPUs? 
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); //which CPU am I?
  ofs  << "Number of tasks= " << numtasks << ".  My rank= " << rank<< endl;

  image.SaveImageData("outData.txt", myData, w, h);
  //outFileG.close();
}

void Transform1D(Complex* h, int w, Complex* H)
{
  // Implement a simple 1-d DFT using the double summation equation
  // given in the assignment handout.  h is the time-domain input
  // data, w is the width (N), and H is the output array.
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
  

  
