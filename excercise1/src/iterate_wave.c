#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <omp.h>

#ifndef RW_PGM
#define RW_PGM
void write_pgm_image_test( void *image, int maxval, int xsize, int ysize, const char *image_name);
#endif
#define MAXVAL 255

#define CPU_TIME (clock_gettime( CLOCK_MONOTONIC, &ts ), (double)ts.tv_sec +	\
		  (double)ts.tv_nsec * 1e-9)

struct Cell{
  long row;
  long col;
};
void read_pgm( unsigned char **image, int *maxval, long *xsize, long *ysize, const char *image_name)
{
  FILE* image_file; 
  image_file = fopen(image_name, "r"); 

  *image = NULL;
  *xsize = *ysize = *maxval = 0;
  
  char    MagicN[2];
  char   *line = NULL;
  size_t  k, n = 0;
  

  k = fscanf(image_file, "%2s%*c", MagicN );


  k = getline( &line, &n, image_file);
  while ( (k > 0) && (line[0]=='#') )
    k = getline( &line, &n, image_file);

  if (k > 0)
    {
      k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
      if ( k < 3 )
	fscanf(image_file, "%d%*c", maxval);
    }
  else
    {

      free( line );
      return;
    }
  free( line );
  
  int color_depth = 1 + ( *maxval > 255 );
  unsigned int size = *xsize * *ysize * color_depth;
  
  if ( (*image = (char*)malloc( size )) == NULL )
    {
      fclose(image_file);
      *maxval = -2;
      *xsize  = 0;
      *ysize  = 0;
      return;
    }
  
  if ( fread( *image, 1, size, image_file) != size )
    {
      free( image );
      image   = NULL;
      *maxval = -3;
      *xsize  = 0;
      *ysize  = 0;
    }  

  fclose(image_file);
  return;
}

//copy all the world 
void copy_world(unsigned char * world, unsigned char * world_prev, long world_size){
  #pragma omp for
  for(long i=0; i<world_size*world_size; i++)
    world_prev[i] = world[i];
}

//Copy the values in the Cells storeed in next update from world to world_prev
void copy_world_partial(unsigned char * world, unsigned char * world_prev, long world_size,struct Cell ** next_update, long size_next_update){
  #pragma omp for
  for(long i=0; i<size_next_update; i++){
    long col = next_update[i]->col;
    long row = next_update[i]->row;
    world_prev[row*world_size+col] = world[row*world_size+col];
  }
}


void iteration(unsigned char * world, unsigned char * world_prev, struct Cell ** next_update,long size, long size_next_update,int it)
{
  #pragma omp for 
  for(int i=0; i<size_next_update; i++){
    long col = next_update[i]->col;
    long row = next_update[i]->row;

    long col_prev = col-1>=0 ? col-1 : size-1;
    long col_next = col+1<size ? col+1 : 0;
    long row_prev = row-1>=0 ? row-1 : size-1;
    long row_next = row+1<size ? row+1 : 0;
    int sum = world[row_prev*size+col_prev]+world[row_prev*size+col]+
      world[row_prev*size+col_next]+world[row*size+col_prev]+world[row*size+col_next]+
      world[row_next*size+col_prev]+world[row_next*size+col]+world[row_next*size+col_next];

    int cond = sum/MAXVAL;

    if(cond==5){
      world_prev[row*size+col]=0;
    }else{
      world_prev[row*size+col] = MAXVAL;
    }

  }
}

//Determine the cell for the next iteration
void initialize_next_update(struct Cell ** next_update, int size_next_update, int it){
    #pragma omp for
    for(int i=0;i<it;i++){
      next_update[i]->row = i; next_update[i]->col = it;
      next_update[i+it]->row = it; next_update[i+it]->col = i;
    }
    next_update[size_next_update-1]->row = it; 
    next_update[size_next_update-1]->col = it;
}

//Compute a single iteration
void update_ordered(unsigned char * world, unsigned char * world_prev, long world_size, struct Cell ** next_update){
  int size_next_update = 1;
  double time,time_copy,time_init;
    struct  timespec ts;
  for(int it = 0;it<world_size;it++){

    size_next_update = it != 0 ? size_next_update+2 : 1;
    
    //Determine the rows that are going to be updated by the next instruction
    initialize_next_update(next_update,size_next_update,it);

    iteration(world,world_prev,next_update,world_size,size_next_update,it);

    copy_world_partial(world_prev,world,world_size,next_update,size_next_update);

  }

}

void run_wave(char * filename, int times, int s ,int * argc, char ** argv[])
{  
  int omp_rank, omp_size;    
  unsigned char * world;
  
  long world_size = 0;
  long ysize = 0;
  int maxval = 0;
  double time_update;
  double tstart_total;
  struct  timespec ts;
  double tstart=CPU_TIME;
  struct Cell ** next_update;

  read_pgm( &world, &maxval, &world_size, &ysize, filename);

  unsigned char * world_prev = (unsigned char *)malloc(world_size*world_size*sizeof(char));

  #pragma omp parallel 
  {

    copy_world(world,world_prev,world_size);

    #pragma omp single
    {
      next_update = (struct Cell **)malloc(sizeof(struct Cell *)*(world_size*2-1));
    }
    
    #pragma omp master
    {
      for(long i=0; i<world_size*2-1;i++){
        next_update[i] = (struct Cell *)malloc(sizeof(struct Cell)*(world_size*2-1));
      }
    }
    
    #pragma omp barrier
    
    for(int i=0; i<times; i++){
      update_ordered(world, world_prev, world_size,next_update);
    }
    #pragma omp master
    {
      char * fname = (char*)malloc(25);
      sprintf(fname, "test/%d_out.pgm",omp_get_max_threads());
      write_pgm_image_test(world,255,world_size,world_size,fname);
      free(world);
      free(next_update);
      free(world_prev);
      printf("%d,%lf\n",omp_get_max_threads(),CPU_TIME-tstart);
    }
  }
}
