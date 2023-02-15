#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>

#ifndef RW_PGM
#define RW_PGM
void write_pgm_image( unsigned char *image, int maxval, long xsize, long ysize, const char *image_name, int rank, int size);
void set_parameters(int rank, int size, long world_size, long * first_row, long * last_row, long * local_rows );
#endif
#define MAXVAL 255


int generate_seed(int omp_rank, int mpi_rank){
  return 2*omp_rank*mpi_rank+omp_rank*omp_rank+mpi_rank*mpi_rank+100;
}

void initialize_parallel(unsigned char * world, long world_size,int size, int rank){
  long local_rows;

  //calculate the number of rows of each MPI Task
  local_rows= world_size%size-rank <= 0 ? (long)(world_size/size) : (long)(world_size/size)+1;

  world = (unsigned char *)malloc(world_size*(local_rows+1)*sizeof(unsigned char));

  MPI_Barrier(MPI_COMM_WORLD);
  
  //Fill the matrix
  #pragma omp parallel
  {
    srand(generate_seed(rank,omp_get_thread_num()));
    #pragma omp for
    for(int i=world_size;i<world_size*(local_rows+1);i++){
      if(rand()%100<70){
        world[i] = 255;
      }else{
        world[i] = 0;
      }
    }
  }
    char * filename = "init";
  write_pgm_image( world, 255, world_size, local_rows, filename, rank, size);
}



void initialize_serial(unsigned char * world, long size){

  world = (unsigned char *)malloc(size*(size+1)*sizeof(unsigned char));

  #pragma omp parallel
  {
    int seed = generate_seed(0,omp_get_thread_num());

    srand(generate_seed(0,omp_get_thread_num()));
    #pragma omp for 
    for(long long i=size; i<size*(size+1); i++){
      int val = rand()%100;
      if(val>70){
        world[i]=0;
      }else{
        world[i]=255;
      }
    }
  }
  
  char * filename = "init";

  write_pgm_image( world, 255, size, size, filename, 0, 1);
}


void initialization(long world_size,const char * filename , int * argc, char ** argv[]){
  
  int rank, size;  
  
  MPI_Status status;
  MPI_Request req;

  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  unsigned char * world;
  if(size > 1){

    initialize_parallel(world,world_size,size,rank);
  }else{
    initialize_serial(world,world_size);
  }
  
  
  char * command = (char *)malloc(50);
  if(rank==0)
  {
    sprintf(command, "cat init_00* > %s",filename);
    system(command);
  
    sprintf(command, "rm init_00*");
    system(command);
  
    free(command);
  }
  MPI_Finalize();
  free(world);
}
