#include <cuda.h>
#include <stdio.h>
#include <stdlib.h>

__global__ void mandelKernel(float lowerX, float lowerY, float stepX, float stepY, int maxIterations,
                             int* Md, int width, int height) {
    
    int Col = blockIdx.x * blockDim.x + threadIdx.x;
    int Row = blockIdx.y * blockDim.y + threadIdx.y;

    float x = lowerX + Col * stepX;
    float y = lowerY + Row * stepY;
    float z_re = x;
    float z_im = y;

    int i;
    for (i = 0; i < maxIterations; ++i)
    {
        if (z_re * z_re + z_im * z_im > 4.f)
            break;

        float new_re = z_re * z_re - z_im * z_im;
        float new_im = 2.f * z_re * z_im;
        z_re = x + new_re;
        z_im = y + new_im;
    }

    Md[Row * width + Col] = i;
}

// Host front-end function that allocates the memory and launches the GPU kernel
void hostFE (float upperX, float upperY, float lowerX, float lowerY, int* __restrict__ img,
             int resX, int resY, int maxIterations)
{   
    float stepX = (upperX - lowerX) / resX;
    float stepY = (upperY - lowerY) / resY;

    int size = sizeof(int) * resX * resY;
    int* __restrict__ M = (int*)malloc (size);

    int* Md;
    cudaMalloc(&Md, size);

    // image: 1600 x 1200
    // limit: (64, 16)
    dim3 dimBlock(16, 16);
    dim3 dimGrid(resX / dimBlock.x, resY / dimBlock.y);

    mandelKernel<<<dimGrid, dimBlock>>>(lowerX, lowerY, stepX, stepY, maxIterations, Md, resX, resY);

    cudaMemcpy(M, Md, size, cudaMemcpyDeviceToHost);

    for (int i = 0; i < resY; ++i)
    {
        for (int j = 0; j < resX; j += 8)
        {
            img[i * resX + j] = M[i * resX + j];
            img[i * resX + j + 1] = M[i * resX + j + 1];
            img[i * resX + j + 2] = M[i * resX + j + 2];
            img[i * resX + j + 3] = M[i * resX + j + 3];
            img[i * resX + j + 4] = M[i * resX + j + 4];
            img[i * resX + j + 5] = M[i * resX + j + 5];
            img[i * resX + j + 6] = M[i * resX + j + 6];
            img[i * resX + j + 7] = M[i * resX + j + 7];
        }
    }

    free(M);
    cudaFree(Md);
}
