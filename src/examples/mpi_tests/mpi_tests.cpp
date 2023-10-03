#include <iostream>
#include <mpi.h>

int main(int argc, char** argv) {
    int world_size;
    int world_rank;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Print off a hello world message
    std::cout << "Hello world from rank " << world_rank << " out of " << world_size << " processes." << std::endl;

    // Finalize the MPI environment
    MPI_Finalize();

    return 0;
}
