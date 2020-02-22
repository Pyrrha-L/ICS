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
	int r, c;						//内部行、列（64*64矩阵转置时作用改变） 
	int row, column; 				//外部行、列   
    int odin,diagval;				//优化对角线上转置时使用的变量 
									//（64*64矩阵转置时作用改变.odin=t6,diagval=t7）
    int t0, t1, t2, t3, t4, t5;				//进行64*64矩阵转置时使用的临时变量 
    
    //主要思想：根据不同的矩阵维度和cache的特性，采用不同的循环展开 
    if(M==32){
    	for (row = 0; row < N; row += 8) {
        for (column = 0; column < M; column += 8) {
            for (r = row; r < row + 8 && r < N; r++) {
                for (c = column; c < column + 8 && c < M; c++) {
                    if (c != r) {//普通的转置
                        B[c][r] = A[r][c];
                    }
                    else {//记录对角线的值 
                        diagval = A[r][c]; 
						odin = r; 
                    }
                }
                if (row == column) {//更新对角线的值 
                    B[odin][odin] = diagval;
                }
            }
        }
    }
	}
	else if(M==64){
	for (row = 0; row < N; row += 8) {
            for (column = 0; column < M; column += 8) {
                //左上角 
                for (r = row; r < row + 4; r++) {
                    t0=A[r][column +0]; t1=A[r][column +1]; t2=A[r][column +2]; t3=A[r][column +3];
                    B[column +0][r]=t0; B[column +1][r]=t1; B[column +2][r]=t2; B[column +3][r]=t3;
                }

                //右上角
                for (r = 0; r < 4; r++) {
                    t0=A[row+r][column +4]; t1=A[row+r][column +5]; t2=A[row+r][column +6]; t3=A[row+r][column +7];
                    B[column +0][row+4+r]=t0; B[column +1][row+4+r]=t1; B[column +2][row+4+r]=t2; B[column +3][row+4+r]=t3;
                }

                //左下角
                for (r = 0; r < 4; r++) {
                    t0=B[column +r][row+4]; t1=B[column +r][row+5]; t2=B[column +r][row+6]; t3=B[column +r][row+7];
                    t4=A[row+4][column +r]; t5=A[row+5][column +r]; odin=A[row+6][column +r]; diagval=A[row+7][column +r];
                    B[column +r][row+4]=t4; B[column +r][row+5]=t5; B[column +r][row+6]=odin; B[column +r][row+7]=diagval;
                    B[column +4+r][row+0]=t0; B[column +4+r][row+1]=t1; B[column +4+r][row+2]=t2; B[column +4+r][row+3]=t3;
                }

                //右下角
                for (r = row + 4; r < row + 8; r++) {
                    t0=A[r][column +4]; t1=A[r][column +5]; t2=A[r][column +6]; t3=A[r][column +7];
                    B[column +4][r]=t0; B[column +5][r]=t1; B[column +6][r]=t2; B[column +7][r]=t3;
                }
            }
        }
	}
	else{
		for (row = 0; row < N; row += 14) {
        for (column = 0; column < M; column += 14) {
            for (r = row; r < row + 14 && r < N; r++) {
                for (c = column; c < column + 14 && c < M; c++) {
                    if (c != r) {
                        B[c][r] = A[r][c];
                    }
                    else {
                        diagval = A[r][c]; 
                        odin = r; 
                    }
                }
                if (row == column) {
                    B[odin][odin] = diagval;
                }
            }
        }
    }
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

