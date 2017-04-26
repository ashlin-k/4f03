CFLAGS=-arch=sm_20
CC=nvcc
TARGET=pa5.x
DEP=ppmFile.h
SOURCE=pa5.cu ppmFile.cu
OBJS=pa5.o ppmFile.o 

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJS): $(SOURCE) $(DEP)
	$(CC) -c $(SOURCE) 

clean:
	rm  *.o $(TARGET)

