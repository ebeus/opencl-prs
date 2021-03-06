// ConsoleApplication3.cpp : Defines the entry point for the console application.
//

/*
Code based on https://www.olcf.ornl.gov/tutorials/opencl-vector-addition/
*/
#include <stdio.h>
#include "stdafx.h"
#include <stdlib.h>
#include <math.h>
#include <CL/opencl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <windows.h>
#include <sstream>
#include <time.h>
#include <chrono>
using std::string;
using namespace std;
using namespace std::chrono;
// OpenCL kernel. Each work item takes care of one element of c

const char *kernelSource = "\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable                    \n" \
"__kernel void vecAdd(  __global float *a,                       \n" \
"                       __global float *b,                       \n" \
"                       __global float *c,                       \n" \
"                       const unsigned int n)                    \n" \
"{                                                               \n" \
"    //Get our global thread ID                                  \n" \
"    int id = get_global_id(0);                                  \n" \
"    //Make sure we do not go out of bounds                      \n" \
"    if (id < n)                                                 \n" \
"        c[id] = a[id] * b[id];                                  \n" \
"}                                                               \n" \
"\n";


int main() {
	// Length of vectors
	//unsigned int n = 5000192;
	unsigned int n = 100000;
	// Host input vectors
	float *h_a;
	float *h_b;
	// Host output vector
	float *h_c;

	// Device input buffers
	cl_mem d_a;
	cl_mem d_b;
	// Device output buffer
	cl_mem d_c;

	cl_platform_id cpPlatform;        // OpenCL platform
	cl_device_id device_id;           // device ID
	cl_context context;               // context
	cl_command_queue queue;           // command queue
	cl_program program;               // program
	cl_program program_suma;
	cl_kernel kernel;                 // kernel
	cl_event prof_event;
	cl_kernel kernel_suma;
									  // Size, in bytes, of each vector
	size_t bytes = n * sizeof(float);
	cl_ulong time_start, time_end;
	// Allocate memory for each vector on host
	h_a = (float*)malloc(bytes);
	h_b = (float*)malloc(bytes);
	h_c = (float*)malloc(bytes);

	// Initialize vectors on host
	int i;
	for (i = 0; i < n; i++)	{
		h_a[i] = (sinf(i)*sinf(i));
		h_b[i] = (cosf(i)*cosf(i));
	}

	size_t globalSize, localSize;
	cl_int err;

	// Number of work items in each local work group
	localSize = 256;

	// Number of total work items - localSize must be devisor
	globalSize = ceil(n / (float)localSize)*localSize;

	// Bind to platform
	err = clGetPlatformIDs(1, &cpPlatform, NULL);

	// Get ID for the device
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

	// Create a context  
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

	// Create a command queue 

	cl_queue_properties props[] = {
		CL_QUEUE_PROPERTIES,
		CL_QUEUE_PROFILING_ENABLE,
		0
	};

	queue = clCreateCommandQueueWithProperties(context, device_id, props, &err);

	// Create the compute program from the source buffer
	program = clCreateProgramWithSource(context, 1,
		(const char **)& kernelSource, NULL, &err);

	if (program == NULL) {
		printf("Error while creating program!\n");
		return 1;
	}

	// Build the program executable 
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err < 0) {
		size_t log_size = 0;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size + 1);
		log[log_size] = '\0';
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size + 1, log, NULL);
		printf("%s\n", log);
		free(log);
		return 1;
	}
	//clBuildProgram(program_suma, 0, NULL, NULL, NULL, NULL);
	// Create the compute kernel in the program we wish to run
	kernel = clCreateKernel(program, "vecAdd", &err);
//	kernel_suma = clCreateKernel(program_suma, "sum", &err); //treba uvesti posebne varijable
	// Create the input and output arrays in device memory for our calculation
	if (err != NULL) {
		printf("Error while creating kernel!\n");
		return 1;
	}

	d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, bytes, NULL, NULL);
	d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, bytes, NULL, NULL);
	d_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, bytes, NULL, NULL);

	// Write our data set into the input array in device memory
	err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0,
		bytes, h_a, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0,
		bytes, h_b, 0, NULL, NULL);

	// Set the arguments to our compute kernel
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
	err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
	err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &n);

	//Pocetak izvrsavanja kernela
	// Execute the kernel over the entire range of the data set  
	err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize,
		0, NULL, &prof_event);

	// Wait for the command queue to get serviced before reading back results
	clFinish(queue);

	// Read the results from the device
	clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0,
		bytes, h_c, 0, NULL, NULL);



	clGetEventProfilingInfo(prof_event, CL_PROFILING_COMMAND_START,
		sizeof(time_start), &time_start, NULL);
	clGetEventProfilingInfo(prof_event, CL_PROFILING_COMMAND_END,
		sizeof(time_end), &time_end, NULL);
	printf("On the device, computation completed in %lu ns. (profiler)\n",
		(time_end - time_start));

	// sekvencijalna suma
	
	//Sum up vector c and print result divided by n, this should equal 1 within error
	float sum = 0;
	printf("Total number of sums: %d \n", n);
	auto begin = std::chrono::high_resolution_clock::now();
	for (i = 0; i<n; i++)
		sum += h_c[i];
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout << "Sequential time: " << duration_cast<std::chrono::nanoseconds>(finish - begin).count() << " ns" << std::endl;
	printf("final result: %f\n", sum);

	// release OpenCL resources
	clReleaseMemObject(d_a);
	clReleaseMemObject(d_b);
	clReleaseMemObject(d_c);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	//release host memory
	free(h_a);
	free(h_b);
	free(h_c);

	return 0;
}
