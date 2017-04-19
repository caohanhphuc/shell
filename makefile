all: mysh

mysh: mysh.c
	gcc -g mysh.c -o mysh -lrt

clean:
	rm mysh
