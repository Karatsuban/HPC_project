# HPC_project

The goal of this project is to optimize the Mandelbrot fractal algorithm for the OpenMP, MPI and CUDA platforms.
The different versions have been executed on a cluster to compare the speedup with the base version.


Here are the instructions to compile and launche the different versions :

## Base version

$ g++ mandelbrot.cpp -o MAND
$ ./MAND file.txt

## OpenMP version

$ icc -qopenmp mandelbrot-OMP.cpp -std=c++11 -o OMP
$ ./OMP file.txt

## MPI version

$ mpicxx mandelbrot-MPI.cpp -std=c++11 -o MPI
$ mpirun -np 256 ./MPI file.txt

## CUDA version

$ nvcc -std=c++11 mandelbrot-CUDA.cu -o CUDA
$ ./CUDA file.txt
