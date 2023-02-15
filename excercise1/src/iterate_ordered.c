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
void write_pgm_image( unsigned char *image, int maxval, int xsize, int ysize, const char *image_name, int rank, int size);
void write_pgm_image_test( void *image, int maxval, int xsize, int ysize, const char *image_name);
#endif


#define MAXVAL 255

#define CPU_TIME (clock_gettime( CLOCK_MONOTONIC, &ts ), (double)ts.tv_sec +	\
		  (double)ts.tv_nsec * 1e-9)


void update_cell(unsigned char * world, long rows, long world_size, long num_local_rows){

    for(long i=world_size; i<world_size*(rows+1); i++){
    long col = i%world_size;
    long r = i/world_size;
    
    long col_prev = col-1>=0 ? col-1 : world_size-1;
    long col_next = col+1<world_size ? col+1 : 0;
    long r_prev = r-1;
    long r_next = r+1;

    int sum = world[r_prev*world_size+col_prev]+world[r_prev*world_size+col]+
      world[r_prev*world_size+col_next]+world[r*world_size+col_prev]+world[r*world_size+col_next]+
      world[r_next*world_size+col_prev]+world[r_next*world_size+col]+world[r_next*world_size+col_next];

    int cond = sum/MAXVAL;

    if(cond==5 || cond==6){
      world[i]=0;
    }else{
      world[i] = MAXVAL;
    }

  }
}

void update_cell_serial(unsigned char * world,long world_size){

  for(long i=0; i<world_size;i++){
    world[i] = world[world_size*world_size+i];
  }
  for(long i=world_size; i<world_size*(world_size+1); i++){
  long col = i%world_size;
  long row = i/world_size;
    
  long col_prev = col-1>=0 ? col-1 : world_size-1;
  long col_next = col+1<world_size ? col+1 : 0;
  long row_prev = row-1;
  long row_next = row+1;

  int sum = world[row_prev*world_size+col_prev]+world[row_prev*world_size+col]+
    world[row_prev*world_size+col_next]+world[row*world_size+col_prev]+world[row*world_size+col_next]+
    world[row_next*world_size+col_prev]+world[row_next*world_size+col]+world[row_next*world_size+col_next];

  int cond = sum/MAXVAL;

  if(cond==5 || cond==6){
    world[i]=0;
  }else{
    world[i] = MAXVAL;
  }
  if(i==world_size){

    for(long j=0; j<world_size;j++){

      world[world_size*(world_size+1)+j] = world[world_size+j];
    }
  }

  }
  char filename[20]; sprintf(filename,"local.pgm");
  write_pgm_image_test(world,255,world_size,world_size+2,filename);
}

void iterate_serial(unsigned char* world, long world_size,int times){
  for(int i=0;i<times;i++){
    update_cell_serial(world,world_size);
  }
}

void iterate(unsigned char* world_local, long world_size, long rows, int rank, int size,int times,MPI_Status * s, MPI_Request * r){

  long num_local_rows=rows+2;
  int tag_odd=0;
  int tag_even=1;

  if(rank==size-1)
    MPI_Isend(&world_local[(rows)*world_size],world_size,MPI_UNSIGNED_CHAR,0,100,MPI_COMM_WORLD,r);


  for(long i=0; i<times; i++){

    if(rank != 0 & rank != size-1){
      MPI_Isend(&world_local[world_size],world_size,MPI_UNSIGNED_CHAR,rank-1,tag_odd,MPI_COMM_WORLD,r);
    }
    if(rank == size-1){
      MPI_Isend(&world_local[world_size],world_size,MPI_UNSIGNED_CHAR,rank-1,tag_odd,MPI_COMM_WORLD,r);
    }


    if(rank == size-1){
      MPI_Recv(world_local,world_size,MPI_UNSIGNED_CHAR,rank-1,tag_odd,MPI_COMM_WORLD,s);
      MPI_Recv(&world_local[(num_local_rows-1)*world_size],world_size,MPI_UNSIGNED_CHAR,0,100,MPI_COMM_WORLD,s);
    }
    if(rank == 0){

      MPI_Recv(world_local,world_size,MPI_UNSIGNED_CHAR,size-1,100,MPI_COMM_WORLD,s);
      MPI_Recv(&world_local[(num_local_rows-1)*world_size],world_size,MPI_UNSIGNED_CHAR,1,tag_odd,MPI_COMM_WORLD,s);
    }
    if(rank != 0 & rank != size-1){
      MPI_Recv(world_local,world_size,MPI_UNSIGNED_CHAR,rank-1,tag_odd,MPI_COMM_WORLD,s);
      MPI_Recv(&world_local[(num_local_rows-1)*world_size],world_size,MPI_UNSIGNED_CHAR,rank+1,tag_odd,MPI_COMM_WORLD,s);
    }

    update_cell(world_local,rows,world_size,num_local_rows);

    
    if(rank == 0){
      MPI_Isend(&world_local[(rows)*world_size],world_size,MPI_UNSIGNED_CHAR,1,tag_odd,MPI_COMM_WORLD,r);
      MPI_Isend(&world_local[world_size],world_size,MPI_UNSIGNED_CHAR,size-1,100,MPI_COMM_WORLD,r);
    }
    if(rank != 0 & rank != size-1){
      MPI_Isend(&world_local[(rows)*world_size],world_size,MPI_UNSIGNED_CHAR,rank+1,tag_odd,MPI_COMM_WORLD,r);
    }
    if(rank == size-1 && i != times-1){
      MPI_Isend(&world_local[(rows)*world_size],world_size,MPI_UNSIGNED_CHAR,0,100,MPI_COMM_WORLD,r);
    }


    MPI_Barrier(MPI_COMM_WORLD);
  }


}

void run_ordered(char * filename, int times, int s ,int * argc, char ** argv[]){

  int rank, size;    
  unsigned char * world;
  
  long world_size = 0;
  long local_size = 0;
  int maxval = 0;    

  MPI_Status status;
  MPI_Request req;

  int mpi_provided_level;

  MPI_Init_thread(argc, argv,MPI_THREAD_FUNNELED,&mpi_provided_level);
  if(mpi_provided_level<MPI_THREAD_FUNNELED){
    printf("Problem when asking MPI_FLUNNED_LEVEL\n");
    MPI_Finalize();
    exit(1);
  }
  double t_start=MPI_Wtime();
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  read_pgm_image( &world, &maxval, &local_size, &world_size,filename,rank,size,&status,&req);

  if(size>1){
    iterate(world, world_size, local_size,rank, size,times,&status, &req);
  }else{
    iterate_serial(world,world_size,times);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  char * fname = (char*)malloc(30);
  sprintf(fname, "output/out");
  write_pgm_image(world,255,world_size,local_size,fname,rank,size);
  free(fname);
  MPI_Barrier(MPI_COMM_WORLD);
  if(rank==0)
    printf("%d,%d,%f\n",size,omp_get_max_threads(), MPI_Wtime()-t_start);
  MPI_Finalize();
  free(world);
}
