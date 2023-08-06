/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
int i,j;
int ii,jj;
                               int temp_value0=0;
		               int temp_value1=0;
		               int temp_value2=0;
		               int temp_value3=0;
		               int temp_value4=0;
		               int temp_value5=0;
		               int temp_value6=0;
		               int temp_value7=0;
if(N==32)
{

    for ( ii = 0; ii < 32; ii+=8) 
        for (jj = 0; jj < 32; jj+=8) 
               for(i=ii;i<ii+8;i++)
		           {
		                temp_value0=A[i][jj];
		                temp_value1=A[i][jj+1];
		               temp_value2=A[i][jj+2];
		                temp_value3=A[i][jj+3];
		               temp_value4=A[i][jj+4];
		                temp_value5=A[i][jj+5];
		               temp_value6=A[i][jj+6];
		                temp_value7=A[i][jj+7];
		               
            B[jj][i] = temp_value0;
            B[jj+1][i] = temp_value1;
            B[jj+2][i] = temp_value2;
            B[jj+3][i] = temp_value3;
            B[jj+4][i] = temp_value4;
            B[jj+5][i] = temp_value5;
            B[jj+6][i] = temp_value6;
            B[jj+7][i] = temp_value7;
		           }
        

}
else if(N==64)
{
  for(ii=0;ii<64;ii+=8){
            for(jj=0;jj<64;jj+=8){
            for(i=jj;i<jj+4;i++)
            {
                temp_value0=A[i][ii];temp_value1=A[i][ii+1];temp_value2=A[i][ii+2];temp_value3=A[i][ii+3];
                temp_value4=A[i][ii+4];temp_value5=A[i][ii+5];temp_value6=A[i][ii+6];temp_value7=A[i][ii+7];
                
                B[ii][i]= temp_value0;
                B[ii][i+4]= temp_value4;
                B[ii+1][i]= temp_value1;
                B[ii+1][i+4]= temp_value5;
                B[ii+2][i]= temp_value2;
                B[ii+2][i+4]= temp_value6;
                B[ii+3][i]= temp_value3;
                B[ii+3][i+4]= temp_value7;
                 
            }
            for(i=ii;i<ii+4;i++)
            {
                               temp_value0=B[i][jj+4];
		               temp_value1=B[i][jj+5];
		               temp_value2=B[i][jj+6];
		               temp_value3=B[i][jj+7];
		               temp_value4=A[jj+4][i];
		               temp_value5=A[jj+5][i];
		               temp_value6=A[jj+6][i];
		               temp_value7=A[jj+7][i];
             
             
                               B[i][jj+4]=temp_value4;
                               B[i][jj+5]=temp_value5;
                               B[i][jj+6]=temp_value6;
                               B[i][jj+7]=temp_value7;
                               B[i+4][jj]=temp_value0;
                               B[i+4][jj+1]=temp_value1;
                               B[i+4][jj+2]=temp_value2;
                               B[i+4][jj+3]=temp_value3;
                               
            
            }
            for(i=ii+4;i<ii+8;i++){
                    temp_value0=A[jj+4][i];temp_value1=A[jj+5][i];temp_value2=A[jj+6][i];temp_value3=A[jj+7][i];

                    B[i][jj+4]=temp_value0;B[i][jj+5]=temp_value1;B[i][jj+6]=temp_value2;B[i][jj+7]=temp_value3;
                }
            
            }
 
}
}
else if(N==61)
{
  for(ii=0;ii<N;ii+=16)
            for(jj=0;jj<M;jj+=16)
                for(i=ii;i<ii+16&&i<N;i++)
                    for(j=jj;j<jj+16&&j<M;j++)
                        B[j][i]=A[i][j];
}


        


      
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

