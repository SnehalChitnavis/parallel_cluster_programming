//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>

#include "RayTrace.h"
#include "master.h"

void masterMain(ConfigData* data)
{
    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    
    //Allocate space for the image on the master.
    float* pixels = new float[3 * data->width * data->height];
    
    //Execution time will be defined as how long it takes
    //for the given function to execute based on partitioning
    //type.
    double renderTime = 0.0, startTime, stopTime;

	//Add the required partitioning methods here in the case statement.
	//You do not need to handle all cases; the default will catch any
	//statements that are not specified. This switch/case statement is the
	//only place that you should be adding code in this function. Make sure
	//that you update the header files with the new functions that will be
	//called.
	//It is suggested that you use the same parameters to your functions as shown
	//in the sequential example below.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //Call the function that will handle this.
            startTime = MPI_Wtime();
            masterSequential(data, pixels);
            stopTime = MPI_Wtime();
            break;
	case PART_MODE_STATIC_STRIPS_VERTICAL:
            //Call the function that will handle this.
            startTime = MPI_Wtime();
            masterStaticStripVertical(data, pixels);
            stopTime = MPI_Wtime();
            break;
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }

    renderTime = stopTime - startTime;
    std::cout << "Execution Time: " << renderTime << " seconds" << std::endl << std::endl;

    //After this gets done, save the image.
    std::cout << "Image will be save to: ";
    std::string file = generateFileName(data);
    std::cout << file << std::endl;
    savePixels(file, pixels, data);

    //Delete the pixel data.
    delete[] pixels; 
}

void masterSequential(ConfigData* data, float* pixels)
{
    //Start the computation time timer.
    double computationStart = MPI_Wtime();

    //Render the scene.
    for( int i = 0; i < data->height; ++i )
    {
        for( int j = 0; j < data->width; ++j )
        {
            int row = i;
            int column = j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * data->width + column );

            //Call the function to shade the pixel.
            shadePixel(&(pixels[baseIndex]),row,j,data);
        }
    }

    //Stop the comp. timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    //After receiving from all processes, the communication time will
    //be obtained.
    double communicationTime = 0.0;

    //Print the times and the c-to-c ratio	
    //This section of printing, IN THIS ORDER, needs to be included in all of the 
    //functions that you write at the end of the function.
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

void masterStaticStripVertical(ConfigData* data, float* pixels)
{
    int tag = 1;
    MPI_Status status;
    	
    float* rec_pixels = new float[(3 * data->width * data->height) + 1];
   
    int rec_size = (3 * data->width * data->height) + 1;

    for (int k = 0; k < (rec_size-1); k++)
    {
	    pixels[k] = 0.0;
    }
    //Start the computation time timer.
    double computationStart = MPI_Wtime();
    int numprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    int new_width = (data->width/numprocs); 

    int x_cols = (data->width%numprocs);   
    
    //Render the scene.
    for( int i = 0; i < data->height; ++i )
    {
        for( int j = 0; j < new_width; ++j )	//Dividing columns into strips 
        {
            int row = i;
            int column = (new_width * data->mpi_rank) + j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * data->width + column );

            //Call the function to shade the pixel.
            shadePixel(&(pixels[baseIndex]),row,column,data);
        }
    }

    if(x_cols != 0)
    {
	    for (int i = 0; i < data->height; i++)
	    {
		 for (int j = 0; j < x_cols; j++)
		 {
		    int row = i;
		    int column = (new_width * numprocs) + j;
		    int baseIndex = 3 * (row * data->width + column );

		    // Call the function to shade the pixel.
		    shadePixel(&(pixels[baseIndex]), row, column, data);
	    
		 }
    
	    }
    }

 
    //Computation calculation

    //Stop the comp. timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    double communicationTime = 0;
    // Receive from all processes and update the pixel array
    for(int i = 1; i < numprocs; i++)
    {
	    //After receiving from all processes, the communication time will
	    //be obtained.
	    double communicationStart = MPI_Wtime();	    
	    //Local buffer to save the pixel array
	    MPI_Recv(rec_pixels, rec_size, MPI_FLOAT, i, tag, MPI_COMM_WORLD, &status);
	    double communicationStop = MPI_Wtime();	    
	    communicationTime = communicationTime + (communicationStop - communicationStart);
   
	    double computationStart = MPI_Wtime();
	    // Update pixels array based on received aray : Pixels array size = Recieved array size - 1
	    for (int j = 0; j < (rec_size - 1); j++)
	    {
		    float o_pixels = pixels[j];
		    pixels[j] = o_pixels + rec_pixels[j];
	    }
	    double computationStop = MPI_Wtime();
	    computationTime = computationTime + (computationStop - computationStart);

	    double computationTime_s = rec_pixels[rec_size];
	    if(computationTime_s > computationTime)
	    {
		    computationTime_s = computationTime;
	    }


    }

    //Print the times and the c-to-c ratio
    //This section of printing, IN THIS ORDER, needs to be included in all of the
    //functions that you write at the end of the function.
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}
