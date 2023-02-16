CC=mpicc -O3 -fopenmp -march=native -g

INCDIR=include
OBJDIR=obj

RW=$(OBJDIR)/read_write_pgm.o 
INIT=$(OBJDIR)/initialize.o 
IT_STATIC=$(OBJDIR)/iterate_static.o 
IT_WAVE=$(OBJDIR)/iterate_wave.o
IT_ORDERED=$(OBJDIR)/iterate_ordered.o
OBJECTS=main.o $(RW) $(INIT) $(IT_STATIC) $(IT_WAVE) $(IT_ORDERED)

main.x: $(OBJECTS)
	mkdir -p output
	mkdir -p snap
	$(CC) $(OBJECTS) -o $@
main.o: main.c
	$(CC) -c main.c
obj/%.o: src/%.c
	mkdir -p obj
	$(CC) -c $^ -o $@
clean:
	rm -rf *.o *.x
	rm -rf obj/*.o
