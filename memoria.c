#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/wait.h>

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

// Cmd_deallocate

// Auxiliares

void * CadenatoPointer (char * s)
{
    void *p;
    sscanf(s, "%p", &p);
    return p;
}

void *DireccionNodoShared(LISTASIMPLE *l, key_t cl) {
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(*l, i);
        if (b == NULL) break;
        if (b->type == SHARED && b->key == cl) {
            return b->addr;
        }
    }
    return NULL;
}

int EliminarNodoDireccion(LISTASIMPLE *l, void *p) {
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(*l, i);
        if (b == NULL) break;
        if (b->addr == p) {
            // BorrarElementoAtPos libera la estructura MemBlock y compacta el array
            return BorrarElementoAtPos(*l, i);
        }
    }
    return -1;
}

void doDeallocateShared (key_t cl)
{
    void *p;                                 /*llamo a la funcion de la lista*/
                                             /*que devuelve la direccion donde*/
    if ((p=DireccionNodoShared (&L,cl))==NULL){/*esta la memoria de clave cl*/
                printf ("No hay bloque de esa clave mapeado en el proceso\n");
                return;
    }
   shmdt(p);
   if (EliminarNodoDireccion (&L,p)==-1)  /*elimino el nodo */
        printf ("Imposible quitar de lista: %s\n",strerror(errno));
}

void do_SharedDelkey (char *args[])
{
   key_t clave;
   int id;
   char *key=args[0];

   if (key==NULL || (clave=(key_t) strtoul(key,NULL,10))==IPC_PRIVATE){
        printf ("      delkey necesita clave_valida\n");
        return;
   }
   if ((id=shmget(clave,0,0666))==-1){
        perror ("shmget: imposible obtener memoria compartida");
        return;
   }
   if (shmctl(id,IPC_RMID,NULL)==-1)
        perror ("shmctl: imposible eliminar memoria compartida\n");
}

// Deallocate principal

void do_DeallocateMalloc(char *tr[]) {
    if (tr[0] == NULL) {
        ImprimirListaMalloc(&L);
        return;
    }

    size_t tam = (size_t)strtoul(tr[0], NULL, 10);
    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(L, i);
        if (b == NULL) break;
        
        if (b->type == MALLOC && b->size == tam) {
            free(b->addr); // 1. Liberamos la memoria del heap
            BorrarElementoAtPos(L, i); // 2. Eliminamos el bloque de la lista
            return;
        }
    }
    printf("No hay bloque de ese tamano asignado con malloc\n");
}

void do_DeallocateMmap(char *tr[]) {
    if (tr[0] == NULL) {
        ImprimirListaMmap(&L);
        return;
    }

    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(L, i);
        if (b == NULL) break;

        if (b->type == MMAP && !strcmp(b->name, tr[0])) {
            if (munmap(b->addr, b->size) == -1) {
                perror("Imposible desmapear fichero");
                return;
            }
            close(b->df);
            EliminarFicheroAbierto(b->df); /* <-- Quita de listopen */
            BorrarElementoAtPos(L, i);
            return;
        }
    }
    printf("Fichero %s no mapeado\n", tr[0]);
}

void do_DeallocateAddr(char *addr_str) {
    void *p = CadenatoPointer(addr_str);

    for (int i = 0; ; i++) {
        MemBlock *b = (MemBlock *)GetElementoAtPos(L, i);
        if (b == NULL) break;

        if (b->addr == p) {
            switch (b->type) {
                case MALLOC:
                    free(b->addr);
                    break;
                case SHARED:
                    if (shmdt(b->addr) == -1) {
                        perror("Imposible desasignar memoria compartida");
                        return;
                    }
                    break;
                case MMAP:
                    if (munmap(b->addr, b->size) == -1) {
                        perror("Imposible desmapear fichero");
                        return;
                    }
                    close(b->df);
                    EliminarFicheroAbierto(b->df); /* <-- Quita de listopen si era un mmap */
                    break;
            }
            BorrarElementoAtPos(L, i);
            return;
        }
    }
    printf("Direccion %p no asignada con malloc, shared o mmap\n", p);
}

void Cmd_deallocate(char *tr[]) {
    if (tr[0] == NULL) {
        ImprimirListaTotal(&L);
        return;
    }

    if (!strcmp(tr[0], "-malloc")) {
        do_DeallocateMalloc(tr + 1);
    } else if (!strcmp(tr[0], "-shared")) {
        if (tr[1] == NULL) {
            ImprimirListaShared(&L);
        } else {
            key_t cl = (key_t) strtoul(tr[1], NULL, 10);
            doDeallocateShared(cl);
        }
    } else if (!strcmp(tr[0], "-delkey")) {
        do_SharedDelkey(tr + 1);
    } else if (!strcmp(tr[0], "-mmap")) {
        do_DeallocateMmap(tr + 1);
    } else {
        do_DeallocateAddr(tr[0]);
    }
}

// Funciones de I/O y memoria

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

// Cmd_memory, muestreo en terminal de datos de memoria, funciones...

// 3 globales inicializadas (.data) y 3 no inicializadas (.bss)
int g_init_1 = 1, g_init_2 = 2, g_init_3 = 3;
int g_noinit_1, g_noinit_2, g_noinit_3;

// 3 estáticas inicializadas (.data) y 3 no inicializadas (.bss)
static int s_init_1 = 10, s_init_2 = 20, s_init_3 = 30;
static int s_noinit_1, s_noinit_2, s_noinit_3;

void do_MemoryVars(void) {
    int auto_1 = 1, auto_2 = 2, auto_3 = 3;

    printf("%-24s%p,    %p,    %p\n", "Variables locales", 
           (void *)&auto_1, (void *)&auto_2, (void *)&auto_3);
    printf("%-24s%p,    %p,    %p\n", "Variables globales", 
           (void *)&g_init_1, (void *)&g_init_2, (void *)&g_init_3);
    printf("%-24s%p,    %p,    %p\n", "Var (N.I.)globales", 
           (void *)&g_noinit_1, (void *)&g_noinit_2, (void *)&g_noinit_3);
    printf("%-24s%p,    %p,    %p\n", "Variables staticas", 
           (void *)&s_init_1, (void *)&s_init_2, (void *)&s_init_3);
    printf("%-24s%p,    %p,    %p\n", "Var (N.I.)staticas", 
           (void *)&s_noinit_1, (void *)&s_noinit_2, (void *)&s_noinit_3);
}

void do_MemoryFuncs(void) {
    printf("%-24s%p,    %p,    %p\n", "Funciones programa", 
           (void *)Cmd_allocate, (void *)Cmd_io, (void *)do_MemoryFuncs);
    printf("%-24s%p,    %p,    %p\n", "Funciones libreria", 
           (void *)printf, (void *)malloc, (void *)open);
}

void Do_pmap (void) /*sin argumentos*/
 { pid_t pid;       /*hace el pmap (o equivalente) del proceso actual*/
   char elpid[32];
   char *argv[4]={"pmap",elpid,NULL};
   
   sprintf (elpid,"%d", (int) getpid());
   if ((pid=fork())==-1){
      perror ("Imposible crear proceso");
      return;
      }
   if (pid==0){
      if (execvp(argv[0],argv)==-1)
         perror("cannot execute pmap (linux, solaris)");
         
      argv[0]="procstat"; argv[1]="vm"; argv[2]=elpid; argv[3]=NULL;   
      if (execvp(argv[0],argv)==-1)/*No hay pmap, probamos procstat FreeBSD */
         perror("cannot execute procstat (FreeBSD)");
         
      argv[0]="procmap",argv[1]=elpid;argv[2]=NULL;    
            if (execvp(argv[0],argv)==-1)  /*probamos procmap OpenBSD*/
         perror("cannot execute procmap (OpenBSD)");
         
      argv[0]="vmmap"; argv[1]="-interleave"; argv[2]=elpid;argv[3]=NULL;
      if (execvp(argv[0],argv)==-1) /*probamos vmmap Mac-OS*/
         perror("cannot execute vmmap (Mac-OS)");      
      exit(1);
  }
  waitpid (pid,NULL,0);
}

void Cmd_memory(char *tr[]) {
    if (tr[0] == NULL || !strcmp(tr[0], "-all")) {
        do_MemoryVars();
        do_MemoryFuncs();
        ImprimirListaTotal(&L);
        return;
    }

    if (!strcmp(tr[0], "-blocks")) {
        ImprimirListaTotal(&L);
    } else if (!strcmp(tr[0], "-funcs")) {
        do_MemoryFuncs();
    } else if (!strcmp(tr[0], "-vars")) {
        do_MemoryVars();
    } else if (!strcmp(tr[0], "-pmap")) {
        Do_pmap();
    } else {
        printf("uso: memory [-blocks|-funcs|-vars|-all|-pmap]\n");
    }
}

// Read y write

void Cmd_read(char *tr[]) {
    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("Parametros incorrectos\n");
        return;
    }

    int df = atoi(tr[0]);
    void *p = CadenatoPointer(tr[1]);
    size_t cont = (size_t) strtoul(tr[2], NULL, 10);

    ssize_t n = read(df, p, cont);
    if (n == -1) {
        printf("Error al leer del descriptor %d a la direccion %p: %s\n", 
               df, p, strerror(errno));
    } else {
        printf("Leidos %ld bytes del descriptor %d en la direccion %p\n", 
               (long)n, df, p);
    }
}

void Cmd_write(char *tr[]) {
    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("Parametros incorrectos\n");
        return;
    }

    int df = atoi(tr[0]);
    void *p = CadenatoPointer(tr[1]);
    size_t cont = (size_t) strtoul(tr[2], NULL, 10);

    ssize_t n = write(df, p, cont);
    if (n == -1) {
        printf("Error al escribir en el descriptor %d desde la direccion %p: %s\n", 
               df, p, strerror(errno));
    } else {
        printf("Escritos %ld bytes en el descriptor %d desde la direccion %p\n", 
               (long)n, df, p);
    }
}

// Recurse

void Recursiva(int n) {
    char automatico[TAMANO];
    static char estatico[TAMANO];

    printf("parametro:%3d(%p) array %p, arr estatico %p\n", 
           n, (void *)&n, (void *)automatico, (void *)estatico);

    if (n > 0)
        Recursiva(n - 1);
}

void Cmd_recurse(char *tr[]) {
    if (tr[0] == NULL) {
        return;
    }
    int n = atoi(tr[0]);
    Recursiva(n);
}

// memfill y memdump

void LlenarMemoria (void *p, size_t cont, unsigned char byte) {
    unsigned char *arr = (unsigned char *) p;
    size_t i;

    for (i = 0; i < cont; i++)
        arr[i] = byte;
}

void Cmd_memfill(char *tr[]) {
    if (tr[0] == NULL){
        printf("parametros incorrectos\n");
        return; 
    }

    void *p = CadenatoPointer(tr[0]);
    size_t cont = 128;
    unsigned char hcode = 'A';
    if(tr[1]!=NULL){
        cont = (size_t)strtoul(tr[1],NULL,10);
    }

    if (tr[2] != NULL) {
        if (tr[2][0] == '\'' || tr[2][0] == '\"') {
            //verifica un caracter literal con comilla/s
            size_t len = strlen(tr[2]);
            if (len == 2) {
                //formato sin comilla de cierre
                hcode = (unsigned char)tr[2][1];
            } else if (len == 3 && (tr[2][2] == '\''||tr[2][2] == '\"')) {
                // y esta con cierre
                hcode = (unsigned char)tr[2][1];
            } else {
                printf("Error: formato de caracter invalido. Inserte un unico caracter (ej. 'C o 'C')\n");
                return;
                //bastante autoexplicativo
            }
        } else if (strncmp(tr[2], "0x", 2) == 0 || strncmp(tr[2], "0X", 2) == 0) {
            //verifica numero hexadecimal si empiza por 0x/0X
            hcode = (unsigned char)strtoul(tr[2], NULL, 16);
        } else {
            //numeros decimales
            hcode = (unsigned char)strtoul(tr[2], NULL, 10);
        }
    }
    printf("Llenando %lu bytes de memoria con el byte %c(0x%02X) a partir de la direccion %p\n",
           (unsigned long)cont, (isprint(hcode) ? hcode : '.'), hcode, p);

    LlenarMemoria(p, cont, hcode);


}

void Cmd_memdump(char *tr[]) {
    // Control de parámetros iniciales (evita desreferenciar NULL)
    if (tr[0] == NULL) {
        printf("parametros incorrectos\n");
        return;
    }

    // Parseo de la dirección con CadenaToPointer
    void *p = CadenatoPointer(tr[0]);
    if (p == NULL) {
        printf("direccion no valida\n");
        return;
    }

    // Cantidad de bytes a volcar (25 bytes por defecto)
    size_t cont = 25;
    if (tr[1] != NULL) {
        cont = (size_t)strtoul(tr[1], NULL, 10);
    }

    unsigned char *arr = (unsigned char *)p;
    printf("Volcando %lu bytes desde la direccion %p\n", (unsigned long)cont, p);

    size_t i = 0;
    while (i < cont) {
        // Ajuste para imprimir en bloques de máximo 25 bytes por tanda
        size_t chunk = (cont - i > 25) ? 25 : (cont - i);

        // Caracteres ASCII (o '.' si no es imprimible)
        for (size_t j = 0; j < chunk; j++) {
            char c = arr[i + j];
            if (isprint((unsigned char)c))
                printf("%4c", c);
            else
                printf("   .");
        }
        printf("\n");

        // Códigos hexadecimales en base 16 de 2 dígitos 
        for (size_t j = 0; j < chunk; j++) {
            printf("  %02x", arr[i + j]);
        }
        printf("\n");

        i += chunk;
    }
}