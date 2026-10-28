CC = gcc
CFLAGS = -g -Wall

shell: Shellv4.c ejemplo.o path.o listasimple.o arquivos.o sistema.o memoria.o
	$(CC) -o shell $(CFLAGS) Shellv4.c  ejemplo.o path.o listasimple.o arquivos.o sistema.o memoria.o

ejemplo.o: ejemplo.c ejemplo.h path.h
	$(CC) -c $(CFLAGS) ejemplo.c

path.o: path.c path.h listasimple.h
	$(CC) -c $(CFLAGS) path.c

listasimple.o: listasimple.c listasimple.h
	$(CC) -c $(CFLAGS) listasimple.c

arquivos.o: arquivos.c arquivos.h listasimple.h
	$(CC) -c $(CFLAGS) arquivos.c

sistema.o: sistema.c sistema.h
	$(CC) -c $(CFLAGS) sistema.c

memoria.o: memoria.c memoria.h
	$(CC) -c $(CFLAGS) memoria.c

clean:
	rm shell *.o
