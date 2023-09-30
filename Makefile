CFLAGS = -Wno-implicit-function-declaration

all: main.o 

main.o: main.c
	gcc $(CFLAGS) -c main.c -Wall -Werror

clean:
	rm main.o 