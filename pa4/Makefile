CFLAGS=-g -Wall
CC=mpicc
TARGET=pa4.x
DEP=ppmFile.h
SOURCE=main.c ppmFile.c
OBJS=main.o ppmFile.o 

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJ): $(SOURCE) $(DEP)
	$(CC) -c $(SOURCE) 

clean:
	rm  *.o $(TARGET)
