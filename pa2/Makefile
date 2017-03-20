INPUT = pa2.cpp
OUTPUT = pa2.x

CC = g++

CFLAGS = -g -Wall -fopenmp


all: $(INPUT)
	@echo creating $(OUTPUT)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(INPUT)

clean: 
	@echo deleting $(OUTPUT)
	rm -f $(OUTPUT) out.txt