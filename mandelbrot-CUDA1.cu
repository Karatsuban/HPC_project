// Raphael Garnier

#include <iostream>
#include <fstream>
#include <complex>
#include <chrono>
#include <cuComplex.h>

// Ranges of the set
#define MIN_X -2
#define MAX_X 1
#define MIN_Y -1
#define MAX_Y 1

// Image ratio
#define RATIO_X (MAX_X - MIN_X)
#define RATIO_Y (MAX_Y - MIN_Y)

// Image size
#define RESOLUTION 2000 # resolution of the output image

#define WIDTH (RATIO_X * RESOLUTION)
#define HEIGHT (RATIO_Y * RESOLUTION)
#define N (HEIGHT * WIDTH)

#define STEP ((double)RATIO_X / WIDTH)

#define DEGREE 2        // Degree of the polynomial
#define ITERATIONS 5000 // Maximum number of iterations on a pixel

using namespace std;


__global__ void GPUFunction(int *imageGPU){
        
    int pos = blockIdx.x * blockDim.x + threadIdx.x; // get the number of the pixel

    if (pos < WIDTH * HEIGHT) // compute only if the pixel number is less than the total of pixels
    {
        imageGPU[pos] = 0; // initialize the value of the pixel

        const int row = pos / WIDTH;
        const int col = pos % WIDTH;

        const cuDoubleComplex cc = make_cuDoubleComplex(col * STEP + MIN_X, row * STEP + MIN_Y);
        
        cuDoubleComplex zz = make_cuDoubleComplex(0,0);
        
        for (int i = 1; i <= ITERATIONS; i++)
        {
            zz = cuCmul(zz, zz); // zz is squared
            zz = cuCadd(zz, cc); // zz is added to cc

            // If it is not convergent
            if (cuCabs(zz) >= 2)
            {
                imageGPU[pos] = i; // store the value
                break;
            }
        }
    }
}


int main(int argc, char **argv)
{
    int* imageGPU;
    dim3 threads(32);
    dim3 blocks((N+threads.x-1)/threads.x);
    const auto start = chrono::steady_clock::now();

    int *const imageCPU = new int[N]; // array storing the pixels on the CPU

    cudaMalloc( (void**)&imageGPU, N*sizeof(int) );  // malloc another array on the GPU

    GPUFunction<<<blocks, threads>>>(imageGPU);  // launch the computation on the GPU

    cudaMemcpy(imageCPU, imageGPU, N*sizeof(int), cudaMemcpyDeviceToHost); // copy data from the GPU to the CPU
    cudaDeviceSynchronize();

    const auto end = chrono::steady_clock::now();
    cout << "Time elapsed: "
         << chrono::duration_cast<chrono::seconds>(end - start).count()
         << " seconds." << endl;

    // Write the result to a file
    ofstream matrix_out;

    if (argc < 2)
    {
        cout << "Please specify the output file as a parameter." << endl;
        return -1;
    }

    matrix_out.open(argv[1], ios::trunc);
    if (!matrix_out.is_open())
    {
        cout << "Unable to open file." << endl;
        return -2;
    }

    for (int row = 0; row < HEIGHT; row++)
    {
        for (int col = 0; col < WIDTH; col++)
        {
            matrix_out << imageCPU[row * WIDTH + col];

            if (col < WIDTH - 1)
                matrix_out << ',';
        }
        if (row < HEIGHT - 1)
            matrix_out << endl;
    }
    matrix_out.close();

    delete[] imageCPU; // It's here for coding style, but useless
    cudaFree(imageGPU); // same, but for cuda
    return 0;
}
