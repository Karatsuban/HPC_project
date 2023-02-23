// Raphael Garnier

#include <iostream>
#include <fstream>
#include <complex>
#include <chrono>
#include <mpi.h>

// Ranges of the set
#define MIN_X -2
#define MAX_X 1
#define MIN_Y -1
#define MAX_Y 1

// Image ratio
#define RATIO_X (MAX_X - MIN_X)
#define RATIO_Y (MAX_Y - MIN_Y)

// Image size
#define RESOLUTION 1000
#define WIDTH (RATIO_X * RESOLUTION)
#define HEIGHT (RATIO_Y * RESOLUTION)

#define STEP ((double)RATIO_X / WIDTH)

#define DEGREE 2        // Degree of the polynomial
#define ITERATIONS 3000 // Maximum number of iterations

#define NB_DATA 500 // Number of adjacents cases a slave will work on

using namespace std;

int main(int argc, char **argv)
{
    int my_rank, comm_size;

    int error = MPI_Init(&argc, &argv); // FIRST MPI call

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size); // init the communications variables
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


    int buff_two[2];  // buffer storing the work instructions
    int buff[NB_DATA + 2];  // other buffer storing the done work

    for(int i=0; i<NB_DATA+2; i++){
		buff[i] = -1; // fills the buffer
    }

    MPI_Status status;  // holds the comm status

    int source;  // source of the comm
    int is_finished = false;  // whetehr there is still work to distribute
    bool is_working = true;  // whether the process is still working

    int curr_pos = 0; // position of the next pixel to do coputations on



	if (my_rank == 0){	// master processor

	    int *const image = new int[HEIGHT * WIDTH]; // initialize the image array
	    int nb_finished = 0;  // nb of finished processors

	    const auto start = chrono::steady_clock::now();

		while (is_working){
			
			// receive a message from a slave
			MPI_Recv(&buff, NB_DATA+2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			source = status.MPI_SOURCE; // get the sender

			if (buff[0] != -1){
				// if the first value is not -1, then it is a message containing values
				for(int i=0; i<buff[1]; i++){
					image[buff[0] + i] = buff[2+i];  // copy the buffer in the image array at the right position
				}
			}

			if (is_finished){ // if the master thread is finished
				nb_finished += 1; // consider the calling slave as finished too
				buff_two[0] = -1; // tell it to stop making requests...
				buff_two[1] = -1; // ...with the message [-1,-1]
			}else{ // the master threas still has work
				buff_two[0] = curr_pos;  // where to begin the computations
				buff_two[1] = NB_DATA;  // nb of pixels on which making the computations

				if(curr_pos+NB_DATA >= WIDTH * HEIGHT){  // we would send too much data to compute
					buff_two[1] = WIDTH*HEIGHT - curr_pos;  // sent "custom" nb_data instead
					is_finished = true; // process 0 has no more work to distribute

				}else{
					curr_pos += NB_DATA; // update the current position
				}
			}

			MPI_Send(&buff_two, 2, MPI_INT, source, 0, MPI_COMM_WORLD); // send the buffer to the calling process


			if (nb_finished == comm_size - 1){
				// if all the slaves are finished
				is_working = false; // the master thread can stop listening to messages
			}
		}

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
	            matrix_out << image[row * WIDTH + col];

	            if (col < WIDTH - 1)
	                matrix_out << ',';
	        }
	        if (row < HEIGHT - 1)
	            matrix_out << endl;
	    }
	    matrix_out.close();

	    delete[] image; // It's here for coding style, but useless

	    MPI_Finalize();
	    return 0;


	}else{ // code for slave process
		while (is_working){

			MPI_Send(&buff, NB_DATA + 2, MPI_INT, 0, 0, MPI_COMM_WORLD); // sends the done work (and asks for more work)
			MPI_Recv(&buff_two, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // receive its work

			int curr_pos = buff_two[0]; // get the coordinate of the first pixel
			int nb_data = buff_two[1]; // get the number of pixels to compute

			if (curr_pos == -1 && nb_data == -1){
				is_working = false; // the slave has received a "stop" message
			}else{
				buff[0] = curr_pos; // storing the values in the answer buffer
				buff[1] = nb_data;
				for (int i=2; i<NB_DATA; i++){
					buff[i] = -1;  // fill the rest of the buffer with -1
				}

				for (int offset=0; offset < nb_data; offset++){
					int pos = curr_pos + offset; // coordinate of the pixel
			        buff[offset + 2] = 0; // initialize the value

			        // do the computations
			        const int row = pos / WIDTH;
			        const int col = pos % WIDTH;
			        const complex<double> c(col * STEP + MIN_X, row * STEP + MIN_Y);

					// z = z^2 + c
					complex<double> z(0, 0);
					for (int i = 1; i <= ITERATIONS; i++)
			        {
						z = pow(z, 2) + c;

						if (abs(z) >= 2)
						{
							buff[offset + 2] = i; // modifies the value in the buffer
							break;
						}
					}
				}
			}
		}
		MPI_Finalize(); // end of the communications
		return 0;  // the slave is not working anymore and stops
	}
}
