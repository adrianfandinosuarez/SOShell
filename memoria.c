#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "memoria.h"
#include "arquivos.h"
#include "listasimple.h"

static LISTASIMPLE L;

// Funciones de nodos y listas




// Funciones de ayuda

void * ObtenerMemoriaShmget (key_t clave, size_t tam)
{
    void * p;
    int aux,id,flags=0777; /*los 9 bits menos significativos de los flags:permisos*/
    struct shmid_ds s;

    if (tam)     /*IMPORTANTE: tam distito de 0 indica crear */
        flags=flags | IPC_CREAT | IPC_EXCL; /*cuando no es crear pasamos de tamano 0*/
    if (clave==IPC_PRIVATE)  /*no nos vale*/
        {errno=EINVAL; return NULL;}
    if ((id=shmget(clave, tam, flags))==-1)
        return (NULL);
    if ((p=shmat(id,NULL,0))==(void*) -1){
        aux=errno;
        if (tam)
             shmctl(id,IPC_RMID,NULL);
        errno=aux;
        return (NULL);
    }
    shmctl (id,IPC_STAT,&s); /* si no es crear, necesitamos el tamano, que es s.shm_segsz*/
    InsertarNodoShared (&L, p, s.shm_segsz, clave); /* DESCOMENTADA */
    return (p);
}

void do_AllocateCreateShared (char *tr[])
{
   key_t cl;
   size_t tam;
   void *p;

   if (tr[0]==NULL || tr[1]==NULL) {
		ImprimirListaShared(&L);
		return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);
   tam=(size_t) strtoul(tr[1],NULL,10);
   if (tam==0) {
	printf ("No se asignan bloques de 0 bytes\n");
	return;
   }
   if ((p=ObtenerMemoriaShmget(cl,tam))!=NULL)
		printf ("Asignados %lu bytes en %p\n",(unsigned long) tam, p);
   else
		printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}

void do_AllocateShared (char *tr[])
{
   key_t cl;
   void *p;

   if (tr[0]==NULL) {
		ImprimirListaShared(&L);
		return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);

   if ((p=ObtenerMemoriaShmget(cl,0))!=NULL)
		printf ("Asignada memoria compartida de clave %lu en %p\n",(unsigned long) cl, p);
   else
		printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}

void * MapearFichero (char * fichero, int protection)
{
    int df, map=MAP_PRIVATE,modo=O_RDONLY;
    struct stat s;
    void *p;

    if (protection&PROT_WRITE)
          modo=O_RDWR;
    if (stat(fichero,&s)==-1 || (df=open(fichero, modo))==-1)
          return NULL;
    if ((p=mmap (NULL,s.st_size, protection,map,df,0))==MAP_FAILED)
           return NULL;
    InsertarNodoMmap (&L,p, s.st_size,df,fichero); /* DESCOMENTADA */
    AnadirFicheroMmap(df, modo, fichero);          /* DESCOMENTADA */
    return p;
}

void do_AllocateMmap(char *arg[])
{ 
     char *perm;
     void *p;
     int protection=0;
     
     if (arg[0]==NULL)
            {ImprimirListaMmap(&L); return;}
     if ((perm=arg[1])!=NULL && strlen(perm)<4) {
            if (strchr(perm,'r')!=NULL) protection|=PROT_READ;
            if (strchr(perm,'w')!=NULL) protection|=PROT_WRITE;
            if (strchr(perm,'x')!=NULL) protection|=PROT_EXEC;
     }
     if ((p=MapearFichero(arg[0],protection))==NULL)
             perror ("Imposible mapear fichero");
     else
             printf ("fichero %s mapeado en %p\n", arg[0], p);
}

// Función implementada, allocate por Malloc

void do_AllocateMalloc(char *tr[]) {
    size_t tam;
    void *p;

    if (tr[0] == NULL) {
        ImprimirListaMalloc(&L);
        return;
    }

    tam = (size_t)strtoul(tr[0], NULL, 10);
    if (tam == 0) {
        printf("No se asignan bloques de 0 bytes\n");
        return;
    }

    p = malloc(tam);
    if (p == NULL) {
        perror("Imposible obtener memoria con malloc");
        return;
    }

    InsertarNodoMalloc(&L, p, tam);
    printf("Asignados %lu bytes en %p\n", (unsigned long)tam, p);
}

void Cmd_allocate(char *tr[]) {
    if (tr[0] == NULL) {
        ImprimirListaTotal(-1);
        return;
    }

    if (!strcmp(tr[0], "-malloc")) {
        do_AllocateMalloc(tr + 1);
    } else if (!strcmp(tr[0], "-createshared")) {
        do_AllocateCreateShared(tr + 1);
    } else if (!strcmp(tr[0], "-shared")) {
        do_AllocateShared(tr + 1);
    } else if (!strcmp(tr[0], "-mmap")) {
        do_AllocateMmap(tr + 1);
    } else {
        printf("uso: allocate [-malloc|-shared|-createshared|-mmap] ....\n");
    }
}

void Cmd_deallocate(char *tr[]) {

}