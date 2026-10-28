#ifndef MEMORIA_H
#define MEMORIA_H

#include <time.h>
#include <sys/types.h> // Para poder usar key_t

#define MAXNAME 1024
#define TAMANO 2048

typedef enum { MALLOC, SHARED, MMAP } TipoMem;

typedef struct {
    void *addr;           // Dirección de memoria devuelta por malloc, shmat o mmap
    size_t size;          // Tamaño en bytes del bloque
    time_t time;          // Instante de la reserva (time(NULL))
    TipoMem type;         // Tipo: MALLOC, SHARED o MMAP
    key_t key;            // Clave IPC (solo válido si type == SHARED)
    char name[MAXNAME];   // Nombre del fichero (solo válido si type == MMAP)
    int df;               // Descriptor del fichero (solo válido si type == MMAP)
} MemBlock;

void Cmd_allocate(char *tr[]);
void Cmd_deallocate(char *tr[]);

#endif