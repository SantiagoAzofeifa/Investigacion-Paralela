CC     = gcc
CFLAGS = -O2 -fopenmp -Wall
TARGET = mandelbrot

all: $(TARGET)

$(TARGET): mandelbrot.c stb_image_write.h
	$(CC) $(CFLAGS) -o $(TARGET) mandelbrot.c -lm

clean:
	rm -f $(TARGET) *.png

run: all
	./mandelbrot 4