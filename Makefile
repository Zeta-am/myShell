myShell: main.o myShell_utils.o
	gcc -pthread -o myShell -g main.o myShell_utils.o

main.o: main.c
	gcc -c main.c

myShell_utils.o: myShell_utils.c myShell_utils.h
	gcc -c -pthread myShell_utils.c

.PHONY: clean
clean:
	rm *.o myShell
