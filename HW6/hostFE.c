#include <stdio.h>
#include <stdlib.h>
#include "hostFE.h"
#include "helper.h"

void hostFE(int filterWidth, float *filter, int imageHeight, int imageWidth,
            float *inputImage, float *outputImage, cl_device_id *device,
            cl_context *context, cl_program *program)
{
    cl_command_queue queue;
    queue = clCreateCommandQueue(*context, *device, 0, NULL);

    const int imageSize = imageWidth * imageHeight;
    const int filterSize = filterWidth * filterWidth;
    cl_mem input = clCreateBuffer(*context, 0, imageSize * sizeof(float), NULL, NULL);
    cl_mem output = clCreateBuffer(*context, 0, imageSize * sizeof(float), NULL, NULL);    
    cl_mem _filter = clCreateBuffer(*context, 0, filterSize * sizeof(float), NULL, NULL);

    clEnqueueWriteBuffer(queue, input, CL_MEM_READ_ONLY, 0, imageSize * sizeof(float), inputImage, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, _filter, CL_MEM_READ_ONLY, 0, filterSize * sizeof(float), filter, 0, NULL, NULL);

    cl_kernel kernel;
    kernel = clCreateKernel(*program, "convolution", NULL);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
    clSetKernelArg(kernel, 1, sizeof(int), &imageHeight);
    clSetKernelArg(kernel, 2, sizeof(int), &imageWidth);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &output);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &_filter);
    clSetKernelArg(kernel, 5, sizeof(int), &filterWidth);

    size_t globalSize[2] = {imageWidth, imageHeight};
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, output, CL_TRUE, 0, imageSize * sizeof(float), outputImage, 0, NULL, NULL);

    clReleaseMemObject(input);
    clReleaseMemObject(output);

}