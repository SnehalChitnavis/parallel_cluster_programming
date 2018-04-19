//This file contains the code that the master process will execute .

#include <iostream>
#include <mpi.h>
#include "RayTrace.h"
#include "slave.h"

void slaveMain(ConfigData* data)
{
    //Allocate space for image on the slave
    float* my_pixels = new float[(3 * data->width * data->height) + 1];
    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //The slave will do nothing since this means sequential operation.
            break;
	case PART_MODE_STATIC_STRIPS_VERTICAL:
	    slaveStaticStripVertical(data, my_pixels);
	    break; 
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented. Process: "<< data->mpi_rank << std::endl;
            break;
    }
}

void slaveStaticStripVertical(ConfigData* data, float* my_pixels)
{
	int tag = 1;
    	int numprocs;
       	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);	
	int new_width = (data->width / numprocs);

    	for (int k = 0; k <  ((3 * data->width * data->height) + 1); k++)
	{
		my_pixels[k] = 0.0;
	}
	//Start the computation time timer.
    	double computationStart = MPI_Wtime();
	
	// Render the scene
	for( int i = 0; i < data->height; ++i)
	{
		for( int j = 0; j < new_width; ++j)
		{
			int row = i;
			int column = (new_width * data->mpi_rank) + j;

			// Calculate the index into the array.
			int baseIndex = 3 * ( row * data->width + column);

			// Call the function to shade the pixel.
			shadePixel(&(my_pixels[baseIndex]), row, column, data);
	    
		}
	}
	
	//Computation calculation

    	//Stop the comp. timer
	double computationStop = MPI_Wtime();
	double computationTime = computationStop - computationStart;

	my_pixels[(3 * data->width * data->height) + 1] = computationTime;

	// Send my_pixels array to master
	MPI_Send(my_pixels, ((3 * data->width * data->height) + 1), MPI_FLOAT, 0, tag, MPI_COMM_WORLD);
}
