// ConsoleApplication3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <chrono>

using namespace std::chrono;

int main(int argc, wchar_t *argv[]) {
	// Length of vectors
	//unsigned int n = 5000192;
	unsigned int n = 10000000;
	double *h_a;
	double *h_b;
	double *h_c;


	int bytes = n * sizeof(double);

	// Allocate memory for each vector on host
	h_a = (double*)malloc(bytes);
	h_b = (double*)malloc(bytes);
	h_c = (double*)malloc(bytes);
	int i;

	for (i = 0; i < n; i++)
	{
		h_a[i] = sinf(i)*sinf(i);
		h_b[i] = cosf(i)*cosf(i);
	}


	auto begin = std::chrono::high_resolution_clock::now();

	for (i = 0; i < n; i++)
		h_c[i] = h_a[i] * h_b[i];
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout <<"Multiplication of a and b completed in: "<< duration_cast<std::chrono::nanoseconds>(finish - begin).count() << " ns" << std::endl;

	//Sum up vector c and print result divided by n, this should equal 1 within error
	double sum = 0;
	begin = std::chrono::high_resolution_clock::now();
	for (i = 0; i<n; i++)
		sum += h_c[i];
	finish = std::chrono::high_resolution_clock::now();
	std::cout << "Summation time: " << duration_cast<std::chrono::nanoseconds>(finish - begin).count() << " ns" << std::endl;
	printf("final result: %f\n", sum);

	// release OpenCL resources

	return 0;
}
