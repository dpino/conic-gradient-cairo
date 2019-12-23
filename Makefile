all:
	gcc main.c -lm `pkg-config --cflags --libs gtk+-3.0` -o main

clean:
	rm -Rf *.o main
