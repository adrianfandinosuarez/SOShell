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

void InsertarNodoShared(LISTASIMPLE *l, void *p, size_t tam, key_t clave) {
    MemBlock *b = malloc(sizeof(MemBlock));
    if (!b) return;
    b->addr = p;
    b->size = tam;
    b->time = time(NULL);
    b->type = SHARED;
    b->key = clave;
    b->df = -1;
    b->name[0] = '\0';
    if (AniadirElemento(*l, b) == -1) free(b);
}

void InsertarNodoMmap(LISTASIMPLE *l, void *p, size_t tam, int df, char *fichero) {
    MemBlock *b = malloc(sizeof(MemBlock));
    if (!b) return;
    b->addr = p;
    b->size = tam;
    b->time = time(NULL);
    b->type = MMAP;
    b->key = 0;
    b->df = df;
    strncpy(b->name, fichero, MAXNAME - 1);
    b->name[MAXNAME - 1] = '\0';
    if (AniadirElemento(*l, b) == -1) free(b);
}

void InsertarNodoMalloc(LISTASIMPLE *l, void *p, size_t tam) {
    MemBlock *b = malloc(sizeof(MemBlock));
    if (!b) return;
    b->addr = p;
    b->size = tam;
    b->time = time(NULL);
    b->type = MALLOC;
    b->key = 0;
    b->df = -1;
    b->name[0] = '\0';
    if (AniadirElemento(*l, b) == -1) free(b);
}

static void ImprimirBloque(MemBlock *b) {
    char fecha[32];
    struct tm *tm_info = localtime(&b->time);
    
    // %e coloca un espacio en blanco para días del 1 al 9 (ej. "Nov  4")
    strftime(fecha, sizeof(fecha), "%b %e %H:%M", tm_info);

    if (b->type == MALLOC) {
        printf("%20p%17lu %s malloc\n", b->addr, (unsigned long)b->size, fecha);
    } else if (b->type == SHARED) {
        printf("%20p%17lu %s shared (key %d)\n", b->addr, (unsigned long)b->size, fecha, (int)b->key);
    } else if (b->type == MMAP) {
        // Dos espacios antes de (descriptor %d)
        printf("%20p%17lu %s %s  (descriptor %d)\n", b->addr, (unsigned long)b->size, fecha, b->name, b->df);
    }
}

void ImprimirListaShared(LISTASIMPLE *l) {
    printf("******Lista de bloques asignados shared para el proceso %d\n", (int)getpid());
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(*l, i);
        if (b == NULL) break; // Fin de la lista
        if (b->type == SHARED) ImprimirBloque(b);
    }
}

void ImprimirListaMmap(LISTASIMPLE *l) {
    printf("******Lista de bloques asignados mmap para el proceso %d\n", (int)getpid());
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(*l, i);
        if (b == NULL) break; // Fin de la lista
        if (b->type == MMAP) ImprimirBloque(b);
    }
}

void ImprimirListaMalloc(LISTASIMPLE *l) {
    printf("******Lista de bloques asignados malloc para el proceso %d\n", (int)getpid());
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(*l, i);
        if (b == NULL) break; // Fin de la lista
        if (b->type == MALLOC) ImprimirBloque(b);
    }
}

void ImprimirListaTotal(LISTASIMPLE *l) {
    printf("******Lista de bloques asignados para el proceso %d\n", (int)getpid());
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(*l, i);
        if (b == NULL) break; // Fin de la lista
        ImprimirBloque(b);
    }
}

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
        ImprimirListaTotal(&L); /* CORREGIDO: antes tenías ImprimirListaTotal(-1); */
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

void * CadenatoPointer (char * s)
{
    void *p;
    sscanf(s, "%p", &p);
    return p;
}

ssize_t LeerFichero (char *f, void *p, size_t cont)
{
   struct stat s;
   ssize_t  n;  
   int df,aux;

   if (stat (f,&s)==-1 || (df=open(f,O_RDONLY))==-1)
	return -1;     
   if (cont==-1)   /* si pasamos -1 como bytes a leer lo leemos entero*/
	cont=s.st_size;
   if ((n=read(df,p,cont))==-1){
	aux=errno;
	close(df);
	errno=aux;
	return -1;
   }
   close (df);
   return n;
}

void do_IOReadFile (char *ar[])
{
   void *p;
   size_t cont=-1;  /*si no pasamos tamano se lee entero */
   ssize_t n;
   if (ar[0]==NULL || ar[1]==NULL){
	printf ("faltan parametros\n");
	return;
   }
   p=CadenatoPointer(ar[1]);  /*convertimos de cadena a puntero*/
   if (ar[2]!=NULL)
	cont=(size_t) atoll(ar[2]);

   if ((n=LeerFichero(ar[0],p,cont))==-1)
	perror ("Imposible leer fichero");
   else
	printf ("leidos %lld bytes de %s en %p\n",(long long) n,ar[0],p);
}

ssize_t EscribirFichero (char *f, void *p, size_t cont, int overwrite)
{
    ssize_t n;
    int df, aux;
    int flags = O_WRONLY | O_CREAT;

    // Si overwrite es 1 (-o presente), truncamos; si es 0, O_EXCL hace fallar si ya existe
    if (overwrite)
        flags |= O_TRUNC;
    else
        flags |= O_EXCL;

    if ((df = open(f, flags, 0666)) == -1)
        return -1;

    if ((n = write(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}

void do_IOWriteFile (char *ar[])
{
    int overwrite = 0;
    int i = 0;
    void *p;
    size_t cont;
    ssize_t n;

    // Comprobamos si el flag -o está presente
    if (ar[i] != NULL && !strcmp(ar[i], "-o")) {
        overwrite = 1;
        i++;
    }

    if (ar[i] == NULL || ar[i+1] == NULL || ar[i+2] == NULL) {
        printf("faltan parametros\n");
        return;
    }

    char *fichero = ar[i];
    p = CadenatoPointer(ar[i+1]);
    cont = (size_t) atoll(ar[i+2]);

    if ((n = EscribirFichero(fichero, p, cont, overwrite)) == -1)
        perror("Imposible escribir fichero");
    else
        printf("escritos %lld bytes en %s desde %p\n", (long long) n, fichero, p);
}

void Cmd_io(char *tr[]) {
    if (tr[0] == NULL) {
        printf("uso: i-o [read|write] ......\n");
        return;
    }

    if (!strcmp(tr[0], "read")) {
        do_IOReadFile(tr + 1);
    } else if (!strcmp(tr[0], "write")) {
        do_IOWriteFile(tr + 1);
    } else {
        printf("uso: i-o [read|write] ......\n");
    }
}