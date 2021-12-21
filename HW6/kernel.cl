__kernel void convolution(__global float* sourceImage,
                          int rows, int cols,
                          __global float* outputImage, 
                          __constant float* filter, 
                          int filterSize)
{
   int col = get_global_id(0);
   int row = get_global_id(1);

   int halfFilterSize = filterSize >> 1;
   float sum = 0.0;

   for(int k = -halfFilterSize; k <= halfFilterSize; ++k) {  
      for(int l = -halfFilterSize; l <= halfFilterSize; ++l) {
         sum += sourceImage[(row + k) * cols + (col + l)] *
               filter[(k + halfFilterSize) * filterSize + (l + halfFilterSize)];
      }
   }
    
   outputImage[row * cols + col] = sum;
}
