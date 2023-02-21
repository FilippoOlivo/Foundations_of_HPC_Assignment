#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <omp.h>


#ifndef RW_PGM
#define RW_PGM
void read_pgm_image( unsigned char **image, int *maxval, long *local_rows, long *world_size, 
                    const char *image_name, int rank, int size, MPI_Status * s, MPI_Request * r);
void write_pgm_image( void *image, int maxval, int xsize, int ysize, const char *image_name, int rank, int size);
void write_pgm_image_test( void *image, int maxval, int xsize, int ysize, const char *image_name);
#endif
#define MAXVAL 255

void update_parallel_static(int rank, unsigned char * world1, unsigned char * world2, long long world_size, int it, long local_rows, int size,
            MPI_Status * s, MPI_Request * r){

  #pragma omp master
  {
  //tags definition
    int tag_odd = 2*it;
    int tag_even = 2*it+1;

    //each process send his fist and last row to respectively the process with rank-1 and rank + 1.
    //Process 0 send his fist line to process size -1 
    //Process size-1 send his last row to process 0
    if(rank == size-1){
      MPI_Isend(&world1[world_size],world_size,MPI_UNSIGNED_CHAR,rank-1,tag_odd,MPI_COMM_WORLD,r);
      MPI_Isend(&world1[(local_rows)*world_size],world_size,MPI_UNSIGNED_CHAR,0,tag_even,MPI_COMM_WORLD,r);
      MPI_Recv(world1,world_size,MPI_UNSIGNED_CHAR,rank-1,tag_even,MPI_COMM_WORLD,s);
      MPI_Recv(&world1[(local_rows+1)*world_size],world_size,MPI_UNSIGNED_CHAR,0,tag_odd,MPI_COMM_WORLD,s);
    }
    if(rank == 0){
      MPI_Isend(&world1[(local_rows)*world_size],world_size,MPI_UNSIGNED_CHAR,1,tag_even,MPI_COMM_WORLD,r);
      MPI_Isend(&world1[world_size],world_size,MPI_UNSIGNED_CHAR,size-1,tag_odd,MPI_COMM_WORLD,r);
      MPI_Recv(world1,world_size,MPI_UNSIGNED_CHAR,size-1,tag_even,MPI_COMM_WORLD,s);
      MPI_Recv(&world1[(local_rows+1)*world_size],world_size,MPI_UNSIGNED_CHAR,1,tag_odd,MPI_COMM_WORLD,s);
    }
    if(rank != 0 & rank != size-1){
      MPI_Isend(&world1[(local_rows)*world_size],world_size,MPI_UNSIGNED_CHAR,rank+1,tag_even,MPI_COMM_WORLD,r);
      MPI_Isend(&world1[world_size],world_size,MPI_UNSIGNED_CHAR,rank-1,tag_odd,MPI_COMM_WORLD,r);
      MPI_Recv(&world1[(local_rows+1)*world_size],world_size,MPI_UNSIGNED_CHAR,rank+1,tag_odd,MPI_COMM_WORLD,s);
      MPI_Recv(world1,world_size,MPI_UNSIGNED_CHAR,rank-1,tag_even,MPI_COMM_WORLD,s);
    }
  }

  #pragma omp for schedule(static,1)
  for(long long i=world_size; i<world_size*(local_rows+1); i++){

    //Calculate position of the actual cell
    long col = i%world_size;
    long r = i/world_size;
    
    //Calculate the neightbours
    long col_prev = col-1>=0 ? col-1 : world_size-1;
    long col_next = col+1<world_size ? col+1 : 0;
    long r_prev = r-1;
    long r_next = r+1;

    //Determine the number of dead neightbours
    int sum = world1[r_prev*world_size+col_prev]+
              world1[r_prev*world_size+col]+
              world1[r_prev*world_size+col_next]+
              world1[r*world_size+col_prev]+
              world1[r*world_size+col_next]+
              world1[r_next*world_size+col_prev]+
              world1[r_next*world_size+col]+
              world1[r_next*world_size+col_next];
    int cond = sum/MAXVAL;
    
    //Update the cell
    world2[i] = MAXVAL;
    if(cond>=5 & cond<=6)
      world2[i]=0;
  }
  #pragma omp barrier
}

void update_serial(unsigned char * world, unsigned char * world_prev,long size){

  #pragma omp for schedule(static,1)
  for(long i=0; i<size;i++){
    world[i] = world[size*size+i];
    world[size*(size+1)+i] = world[size+i];
  }
  

  #pragma omp for schedule(static,1)
  for(int i=size; i<size*(size+1); i++){

    //Calculate position of the actual cell
    long col = i%size;
    long row = i/size;

    //Calculate the neightbours
    long col_prev = col-1>=0 ? col-1 : size-1;
    long col_next = col+1<size ? col+1 : 0;
    long row_prev = row-1;
    long row_next = row+1;

    //Determine the number of dead neightbours
    int sum = world[row_prev*size+col_prev]+world[row_prev*size+col]+
      world[row_prev*size+col_next]+world[row*size+col_prev]+world[row*size+col_next]+
      world[row_next*size+col_prev]+world[row_next*size+col]+world[row_next*size+col_next];
    int cond = sum/MAXVAL;
    world_prev[i] = MAXVAL;
    if(cond>=5 & cond<=6)
      world_prev[i]=0;
  }
  
}

void iterate_static(const int rank, const int size, unsigned char ** world, 
                              const long world_size, const long local_rows, const int times, const int s,
                              MPI_Status * status, MPI_Request * req){

  unsigned char * world_local = *world;

  //Allocate the memory for the previous state
  unsigned char * world_local_prev = (unsigned char *)malloc(world_size*(local_rows+2)*sizeof(unsigned char) );

  #pragma omp parallel
  {
  for(int i=1; i<=times; i++){ 
    
    if(size>1){
      update_parallel_static(rank,world_local, world_local_prev, world_size,i,local_rows,size,status,req);
    }
    if(size==1){
      update_serial(world_local,world_local_prev,world_size);

    }
    #pragma omp master
    {
      unsigned char * temp = world_local;
      world_local = world_local_prev;
      world_local_prev = temp;
      if(i%s==0){
        char * fname = (char*)malloc(60);
        sprintf(fname, "snap/snap_static_%03d",i);
	      write_pgm_image(world_local,255,world_size,local_rows,fname,rank,size);
        free(fname);
      }
    }
    #pragma omp barrier
  }

}
  //free(world_local);
  free(world_local_prev);
  *world = world_local;
}



void run_static(char * filename, int times, int s ,int * argc, char ** argv[]){

  int rank, size;    
  unsigned char * world;
  
  long world_size = 0;
  long local_size = 0;
  int maxval = 0;    
   //a reduced copy of the world (a different part of the copy to each MPI process
  MPI_Status status;
  MPI_Request req;

  int mpi_provided_level;

  MPI_Init_thread(argc, argv,MPI_THREAD_FUNNELED,&mpi_provided_level);
  if(mpi_provided_level<MPI_THREAD_FUNNELED){
    printf("Problem when asking MPI_FLUNNED_LEVEL\n");
    MPI_Finalize();
    exit(1);
  }

  //start time
  double t_start=MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  /*Read the matrix:
    - Calculate the number of local rows
    - Allocate the memory of word (world_size*(local_rows+2)). The first row
      is used to store the value last row of the previous process (size-1 if 
      rank == 0) and the last row is used to store the first row of the previous
      process. In the case of a single MPI Task the first row will store the row
      of of the world and the last line will store the fist row of the world
    - Read the values: the first value is stored in world[world_size]
  */
  read_pgm_image( &world, &maxval, &local_size, &world_size,filename,rank,size,&status,&req);

  iterate_static(rank, size, &world, world_size, local_size, times, s,&status, &req);

  //Wait that all MPI Task finisch the elaboration
  MPI_Barrier(MPI_COMM_WORLD);

  char * fname = (char*)malloc(60);
  sprintf(fname, "output/out_static");
  write_pgm_image(world,255,world_size,local_size,fname,rank,size);
  free(fname);

  MPI_Barrier(MPI_COMM_WORLD);

  if(rank==0)
    printf("%d,%d,%f\n",size,omp_get_max_threads(), MPI_Wtime()-t_start);
  
  MPI_Finalize();
  free(world);
}
