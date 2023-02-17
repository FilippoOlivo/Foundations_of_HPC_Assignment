# Excercise 1
The aim of the excercise is to implement a parallel version of the Conway's Game of Life through MPI and OpenMP. Game of life is a zero player game which evolution depends only on the initial conditions. In this project I have implemented three possible iteration tecniques:

- Static iteration: we freeze the system at each state $s_i$ and then we evaluate the new cell status $s_{i+1}$ based on the system at state $s_i$

- Ordered iteration: we start from the cell (0,0) and we evolve by lines

- Wave iteration: wave that spread in diagonal from cell (0,0)

In src directory you can find the source code.

- iterate_static.c: perform the static evolution
- iterate_ordered.c: perform the ordered evolution
- iterate_wave.c: perform the wave evolution
- initialize.c: perform the initialization of the world
- read_write_pgm.c: file that read and write from/to a .pgm file

## Compilation

To compile the program:

```bash
make
```

Makefile automatically create the folders:

- output: export the final state image
- snap: export the intermidiate steps images
- obj: folder in which object files used during compilation will be places

Your computer must have MPI installed

## Run

You can run the code with ``mpirun``. The input pameters are

- ``-f``: name of the file where you to write/read from (always mandatory)
- ``-k``: size of the world that you want to inizialize 
- ``-s``: every how many steps you want to save a snap of the world (default value 0, save only the output)
- ``-i``: initialize the playground
- ``-r``: run the game
- ``-n``: number of steps
- ``-e``: evolution policy:
    - 0: ordered
    - 1: static
    - 2: wave

After running the program execute the scripts ``joining_snap.sh`` and ``joining_output.sh`` to perform the joining of respectively the partial snap and output files.

## Other files

In the folder times you can find the ``.csv`` files where I have printed the elapsed times of the test that I have performed. In the folder plots you can find the plots of the scalability. In the time folder the files are named as follows:

- time_"architecture"_"size_of_world"_"number_of_MPI_Tasks"_"num_of_iterations".csv
    If the number of iterations is not present it means that the number of iteration is 50 for epyc and 100 for thin
- time_epyc_weak_10000.csv: Time used to produce the weak scalability graph. 10000 state for the staarting size


