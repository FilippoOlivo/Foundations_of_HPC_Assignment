#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <getopt.h>

#ifndef RW_PGM
#define RW_PGM
void write_pgm_image( void *image, int maxval, int xsize, int ysize, const char *image_name);
#endif

#ifndef WORLD
#define WORLD
void initialization(long size,char * filename);
void run_static(char * filename, int times,int s, int * argc, char ** argv[]);
void run_wave(char * filename, int times, int s ,int * argc, char ** argv[]);
void run_ordered(char * filename, int times, int s ,int * argc, char ** argv[]);
#endif

#define SIZE 4800

#define MAXVAL 255

#define INIT 1 //initialize matrix
#define RUN 2

#define ORDERED 0
#define STATIC 1
#define WAVE 2

#define DEFAULT_IT 100

#define S

#define CPU_TIME (clock_gettime( CLOCK_MONOTONIC, &ts ), (double)ts.tv_sec +	\
		  (double)ts.tv_nsec * 1e-9)

#include <time.h>

void sum(unsigned char * world,long size){
  
  int sum = 0;
  for(int i = 0; i<size*size; i++)
    sum += world[i];
}


int main(int argc, char * argv[]){
  int act = 0;
  long world_size = SIZE;

  int dump = 0;
  int action = 0;
  
  char *optstring = "irk:e:f:n:s:h";
  
  int it = DEFAULT_IT;

  int e = STATIC;

  int c;
  
  char * filename;
  
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch(c) {
      
    case 'i':
      action = INIT; break;
      
    case 'r':
      action = RUN; break;
      
    case 'k':
      world_size = atoi(optarg); break;

    case 'e':
      e = atoi(optarg); break;

    case 'f':
      filename = (char*)malloc( sizeof(optarg)+1 );
      sprintf(filename, "%s", optarg );
      break;

    case 'n':
      it = atoi(optarg); break;

    case 's':
      dump = atoi(optarg); break;
    case 'h':
      printf("-i:\t initialize the world\n-r:\t run the game\n-k:\t set world size (defaul value 2000)\n-e:\t type of evolution (0: ordered,1: static, 2: wave)\n-f:\t filename where the world is written (initialization) or read (run)\n-n:\t numeber of steps\n-s:\t how many iteration between two dumps (defaul 0: print only last state)\n");
    break;
    default :
      printf("argument -%c not known\n", c ); break;
    }
  }

  if(action == INIT){
    //printf("Initialize matrix\n");
    printf("num elements = %ld\n",world_size);
    initialization(world_size,filename);
  }
  if(action == RUN & e == STATIC){
    //printf("Iterate world\n");
    run_static(filename,it,dump,&argc,&argv);
  }
  if(action == RUN & e == ORDERED){
    run_ordered(filename,it,dump,&argc,&argv);
  }
  if(action == RUN & e == WAVE){
    run_wave(filename,it,dump,&argc,&argv);
  }

  free(filename);
}

