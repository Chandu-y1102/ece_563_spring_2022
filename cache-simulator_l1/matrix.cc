#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <iomanip>
#include <fstream>

using namespace std;

void multiply(unsigned n);
void block_multiply(unsigned n, unsigned block_size);


void usage(){
	cout << "Usage: ";
	cout << "multiply|block_multiply <n> <block_size>" << endl;
	cout << "<block_size> required only by block_multiply" << endl;
}

/********************************
 *  *      MAIN                    *
 *   ********************************/
int main(int argc, char** argv){
        if (argc < 3){
                usage();
		return -1;
        }
        int n = atoi(argv[2]);
       	if (n<=0){
                usage();
		return -1;
	}
	if (!strcmp(argv[1], "multiply")){
                multiply(n);
        }
        else if (!strcmp(argv[1], "block_multiply")){
                if (argc < 4){
                	usage();
			return -1;
                }
                int block_size = atoi(argv[3]);
		if (block_size <=0 || block_size > n){
			usage();
			return -1;
		}
                block_multiply(n,block_size);
        }else{
                usage();
		return -1;
        }
        return 0;
}


/* This function generates the memory accesses trace for a naive matrix multiplication code (see pseudocode below).
The code computes c=a*b, where a, b and c are square matrices containing n*n single-precision floating-point values.
Assume that the matrices are stored in row-major order, as follows:

a[0,0] a[0,1] ... a[0,n-1] a[1,0] a[1,1] ... a[1,n-1] ... a[n-1,0] a[n-1,1] ... a[n-1,n-1]

Assume that the three matrices are stored contiguously in memory starting at address 0x0.
The trace should contain all memory accesses to matrices a, b and c. You can ignore variables i, j, k and parameter n.
You can ignore the initialization of matrix c (i.e., assume it is already initialized to all 0s.

   for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
	 for (int k = 0; k < n; k++) {
            c[i,j] = a[i,k] * b[k,j] + c[i,j];
         }
      }
   }

The trace should be written to standard output, and it should follow the format required by your cache simulator.
*/
void multiply(unsigned n){
	unsigned mat_size;
unsigned addr_A;
unsigned addr_B;
unsigned addr_C;
unsigned start_address=0x0;
mat_size = n*n*4;
unsigned A[mat_size][mat_size],B[mat_size][mat_size],C[mat_size][mat_size];
addr_A = start_address;
addr_B = start_address+mat_size;
addr_C = start_address+2*mat_size;
ofstream file;
file.open("mat_mul.t");

for (int i = 0;(unsigned) i < n; i++) {
			for (int j = 0; (unsigned)j < n; j++) {
				 for (int k = 0; (unsigned)k < n; k++) {
						C[i][j] += A[i][k] * B[k][j];
						file << "r 0x" << hex << addr_A + i*n*4+k*4 << endl;
						file << "r 0x" << hex << addr_B + j*4+k*n*4 << endl;
						file << "r 0x" << hex << addr_C + i*n*4+j*4 << endl;
						file << "w 0x" << hex << addr_C + i*n*4+j*4  << endl;
				 }
			}
	 }  file.close();
}

/* This functi n generates the memory accesses trace for a matrix multiplication code that uses blocking (see pseudocode below).
The code computes c=a*b, where a, b and c are square matrices containing n*n single-precision floating-point values.
Assume that the matrices are stored in row-major order, as follows:

a[0,0] a[0,1] ... a[0,n-1] a[1,0] a[1,1] ... a[1,n-1] ... a[n-1,0] a[n-1,1] ... a[n-1,n-1]

Assume that the three matrices are stored contiguously in memory starting at address 0x0.
The trace should contain all memory accesses to matrices a, b and c. You can ignore variables ii, jj, kk, i, j, k and parameters n and block_size.
You can ignore the initialization of matrix c (i.e., assume it is already initialized to all 0s.

for (ii = 0; ii < n; ii+=block_size) {
  for (jj = 0; jj < n; jj+=block_size) {
    for (kk = 0; kk < n; kk+=block_size) {
      for (i = ii; i < ii+block_size; i++) {
        for (j = jj; j < jj+block_size; j++) {
          for (k = kk; k < kk+block_size; k++) {
            c[i,j] = a[i,k] * b[k,j] + c[i,j];
          }
        }
      }
    }
  }
}

The trace should be written to standard output, and it should follow the format required by your cache simulator.
*/
void block_multiply(unsigned n, unsigned block_size){
	unsigned mat_size;
unsigned addr_A;
unsigned addr_B;
unsigned addr_C;
unsigned start_address=0x0;
mat_size = n*n*4;
unsigned A[mat_size][mat_size],B[mat_size][mat_size],C[mat_size][mat_size];
addr_A = start_address;
addr_B = start_address+mat_size;
addr_C = start_address+2*mat_size;
ofstream file;
file.open("traces/mat_mul_b1.t");

for (unsigned ii = 0; ii < n; ii+=block_size) {
	for (unsigned jj = 0; jj < n; jj+=block_size) {
		for (unsigned kk = 0; kk < n; kk+=block_size) {
			for (unsigned i = ii; i < ii+block_size; i++) {
				for (unsigned j = jj; j < jj+block_size; j++) {
					for (unsigned k = kk; k < kk+block_size; k++) {
						C[i][j] += A[i][k] * B[k][j];
						file << "r 0x" << hex << addr_A + i*n*4+k*4 << endl;
						file << "r 0x" << hex << addr_B + j*4+k*n*4 << endl;
						file << "r 0x" << hex << addr_C + i*n*4+j*4 << endl;
						file << "w 0x" << hex << addr_C + i*n*4+j*4  << endl;
				 }}}}}}

		 file.close();
}
