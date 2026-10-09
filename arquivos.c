#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>

#include "arquivos.h"
#include "listasimple.h"

static LISTASIMPLE openFiles;

void Cmd_trychdir(char *args[])
{
    int silent = 0;

    if (!strcmp(args[0], "-s")) {
        silent = 1;
        args++;
    }

    char* cadea;
    while ((cadea=*args) != NULL) {
        if (chdir(cadea)==-1) {
            if (!silent) {
                perror(cadea);
            }
        } else {
            if (!silent) {
                printf("%s Ok!\n", cadea);
            }
            return;
        }
        args++;
    }
    printf("Non puiden\n");
}

void ImprimirFichero(void *ptr) {
    FileEntry *f = (FileEntry *)ptr;
    int m = f->mode;

    long offset = lseek(f->df, 0, SEEK_CUR);
    char str_offset[32] = "  "; 

    if (offset != -1) sprintf(str_offset, "%ld", offset);

    printf("descriptor: %d, offset: (%s)-> %s %s%s%s%s%s\n", 
        f->df, 
        str_offset,
        f->name,
        (m & O_ACCMODE) == O_RDWR ? "O_RDWR" : ((m & O_ACCMODE) == O_WRONLY ? "O_WRONLY" : "O_RDONLY"),
        (m & O_CREAT)  ? " O_CREAT" : "",
        (m & O_EXCL)   ? " O_EXCL" : "",
        (m & O_APPEND) ? " O_APPEND" : "",
        (m & O_TRUNC)  ? " O_TRUNC" : ""
    );
}

void InitializeOpenFiles() {
    char *open_files[] = {"entrada estandar", "salida estandar", "error estandar"};
    for (int i = 0; i < 3; i++) {
        FileEntry *f = malloc(sizeof(FileEntry));
        f->df = i;
        f->mode = O_RDWR;
        strcpy(f->name, open_files[i]);
        f->name[MAXNAME-1] = '\0';
        AniadirElemento(openFiles, f);
    }
}

void Cmd_open (char * tr[])
{
    int i,df, mode=0;
    
    if (tr[0]==NULL) { /*no hay parametro*/
        ImprimirListaCompleta(openFiles, 0, ImprimirFichero);
        return;
    }

    for (i=1; tr[i]!=NULL; i++)
      if (!strcmp(tr[i],"cr")) mode|=O_CREAT;
      else if (!strcmp(tr[i],"ex")) mode|=O_EXCL;
      else if (!strcmp(tr[i],"ro")) mode|=O_RDONLY; 
      else if (!strcmp(tr[i],"wo")) mode|=O_WRONLY;
      else if (!strcmp(tr[i],"rw")) mode|=O_RDWR;
      else if (!strcmp(tr[i],"ap")) mode|=O_APPEND;
      else if (!strcmp(tr[i],"tr")) mode|=O_TRUNC; 
      else break;
      
    if ((df=open(tr[0],mode,0777))==-1)
        perror ("Imposible abrir fichero");
    else{
        FileEntry *f = malloc(sizeof(FileEntry));
        f->df = df;
        f->mode = mode;
        strncpy(f->name, tr[0], MAXNAME-1);
        f->name[MAXNAME-1] = '\0';
        if (AniadirElemento(openFiles, f)==-1) {
            perror("Imposible anadir entrada a la tabla ficheros abiertos");
            free(f);
        } else
            printf("Anadida entrada a la tabla ficheros abiertos. %s abierto con descriptor %d\n", tr[0], df);
    }
}

int CompFE(void *elemento_lista, void *descriptor_buscado) {

    FileEntry *f = (FileEntry *)elemento_lista;
    int *df = (int *)descriptor_buscado;
    
    if (f->df == *df) {
        return 0;
    }
    
    return 1;
}

void Cmd_close (char *tr[])
{ 
    int df;
    int fClose = 0;
    
    if (tr[0] == NULL || (df = atoi(tr[0])) < 0) {
        ImprimirListaCompleta(openFiles, 0, ImprimirFichero);
        return;
    }

    if (tr[1] != NULL && !strcmp(tr[1], "-f")) {
        fClose = 1; // Duda con respecto a implementar -f
        if (fClose) {
            printf("Se trata de forzar cierre del descriptor %d\n", df);
        }
    }

    
    if (close(df)==-1)
        perror("Imposible cerrar descriptor");
    else {
        if (BorrarElemento(openFiles, &df, CompFE)==-1)
            perror("Imposible borrar entrada de la tabla ficheros abiertos");
        else
            printf("Descriptor %d cerrado y eliminado de la tabla ficheros abiertos\n", df);
    }
}

void Cmd_listopen(char *tr[])
{
    ImprimirListaCompleta(openFiles, 0, ImprimirFichero);
}

char *DescriptorFileName(int df)
{

    int pos = BuscarElemento(openFiles,&df,CompFE);

    if (pos==-1){
        perror("Descriptor no encontrado");
                return NULL;
    }
    return openFiles[pos] != NULL ? ((FileEntry *)openFiles[pos])->name : NULL;
}

void Cmd_dup (char * tr[])
{ 
    int df, duplicado;
    char aux[MAXNAME],*p;
    
    if (tr[0]==NULL || (df=atoi(tr[0]))<0) { /*no hay parametro*/
        ImprimirListaCompleta(openFiles, 0, ImprimirFichero);        /*o el descriptor es menor que 0*/
        return;
    }
    
 
    p=DescriptorFileName(df);

    if (p==NULL) {
        perror("Descriptor no encontrado en la tabla de ficheros abiertos\n");
        return;
    }

    duplicado=dup(df);
    if (duplicado==-1) {
        perror("Imposible duplicar descriptor");
        return;
    }

    sprintf (aux,"dup %d (%s)",df, p);

    FileEntry *f = malloc(sizeof(FileEntry));
    f->df = duplicado;
    f->mode = fcntl(duplicado,F_GETFL);
    strncpy(f->name, aux, MAXNAME-1);
    f->name[MAXNAME-1] = '\0';
    if (AniadirElemento(openFiles, f)==-1) {
        perror("Imposible anadir entrada a la tabla ficheros abiertos");
        free(f);
    } else
        printf("Anadida entrada a la tabla ficheros abiertos. Descriptor %d abierto con descriptor %d\n", df, duplicado);
}

void Cmd_lseek (char * tr[])
{
    int df;
    long pos, res;
    int ref = SEEK_SET;

    if (tr[0] == NULL || tr[1] == NULL) {
        printf("Parametros incorrectos\n");
        return;
    }

    df = atoi(tr[0]);
    if (df < 0 || (df == 0 && tr[0][0] != '0')) {
        printf("Parametros incorrectos\n");
        return;
    }

    pos = atol(tr[1]);

    if (tr[2] != NULL) {
        if (!strcmp(tr[2], "SEEK_SET")) {
            ref = SEEK_SET;
        } else if (!strcmp(tr[2], "SEEK_CUR")) {
            ref = SEEK_CUR;
        } else if (!strcmp(tr[2], "SEEK_END")) {
            ref = SEEK_END;
        } 
    }

    res = lseek(df, pos, ref);
    
    if (res == -1) {
        char err_msg[256];
        sprintf(err_msg, "Error al intentar posicionar el descriptor %d en el offset %ld", df, pos);
        perror(err_msg);
    } else {
        printf("Descriptor %d posicionado en %ld\n", df, res);
    }
}

void Cmd_readstr(char *tr[])    
{
    int df;
    size_t cont;
    ssize_t nbytes;
    char *buffer;

    if (tr[0] == NULL || tr[1] == NULL) {
        printf("Faltan parametros\n");
        return;
    }

    df = atoi(tr[0]);
    cont = (size_t) atoi(tr[1]);

    if (df < 0 || (int)cont <= 0) {
        printf("Descriptor o contador no valido\n");
        return;
    }

    buffer = malloc(cont + 1);
    if (buffer == NULL) {
        perror("Imposible reservar memoria para readstr");
        return;
    }

    nbytes = read(df, buffer, cont);
    if (nbytes == -1) {
        perror("Imposible leer del descriptor");
        free(buffer);
        return;
    }
    //Asegurar el fin de cadena y mostrar por pantalla
    buffer[nbytes] = '\0';

    printf("Leídos %ld bytes: %s\n", (long)nbytes, buffer);
    
    free(buffer);
}

void Cmd_writestr (char * tr[])
{
    int df;
    ssize_t writen;

    if (tr[0] == NULL || tr[1] == NULL) {
        printf("Faltan parametros\n");
        return;
    }

    df = atoi(tr[0]);
    if (df < 0 ) {
        printf("Parametros incorrectos\n");
        return;
    }

    writen = write(df, tr[1], strlen(tr[1]));

    if (writen == -1) {
        char err_msg[256];
        sprintf(err_msg, "Error intentar escribir %ld bytes en el descriptor %d", (long)strlen(tr[1]), df);
        perror(err_msg);
    } else {
        printf("Escritos %ld bytes en el descriptor %d\n", (long)writen, df);
    }
}

void Cmd_makefile (char *tr[]){

    int df;
    char *filename;

    if (tr[0] == NULL) {
        printf("Falta el nombre del fichero a crear.\n");
        return;
    }

    filename = tr[0];

    df = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (df == -1) {
        perror("Imposible crear fichero");
        return;
    }

    printf("Fichero %s creado con descriptor %d\n", filename, df);
    close(df);
}

void Cmd_makedir (char * tr[])
{
    if (tr[0] == NULL) {
        printf("Falta el nombre del directorio\n");
        return;
    }

    if (mkdir(tr[0], 0777) == -1) {
        perror("Imposible crear directorio");
    } else {
        printf("Directorio %s creado con exito\n", tr[0]);
    }
}

void Cmd_delete (char *tr[]){
    int i;

    if (tr[0] == NULL) {
        printf("Falta el nombre del fichero a eliminar.\n");
        return;
    }


    for (i = 0; tr[i] != NULL; i++){
        if (unlink(tr[i]) == -1) { //borrar como fichero
           if (errno == EISDIR) { //En caso de directorio
                if (rmdir(tr[i]) == -1) {
                    perror(tr[i]); // Si falla el rmdir 
                }
              } else {
                perror(tr[i]); // Si falla el unlink por otro motivo, muestra el error
              }
        } 
    }
}

int borrar_recursivo(const char *ruta) {
    DIR *dir;
    struct dirent *entrada;
    char ruta_completa[1024];
    struct stat st;

    dir = opendir(ruta);//Abrir el directorio
    if (dir == NULL) {
        return -1; //error de apertura
    }

    //Leer cada elemento de dentro del directorio
    while ((entrada = readdir(dir)) != NULL) {
        // Ignorar las entradas "." y ".."
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        
        // Construir la ruta completa del fichero/subdirectorio hijo
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", ruta, entrada->d_name);

        // Comprobar si es un directorio o un archivo normal
        if (lstat(ruta_completa, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Si es un subdirectorio, llamada recursiva
                if (borrar_recursivo(ruta_completa) == -1) {
                    closedir(dir);
                    return -1;
                }
            } else {
                // Si es un fichero normal (u otro enlace), se borra con unlink
                if (unlink(ruta_completa) == -1) {
                    closedir(dir);
                    return -1;
                }
            }
        }
    }

    closedir(dir);

    // 3. Una vez vaciado por completo, borramos el directorio padre con rmdir
    return rmdir(ruta);
}

// Funcion principal del comando para el shell
void Cmd_deltree(char *tr[]) {
    int i = 0;

    if (tr[0] == NULL) {
        printf("Parametro incorrecto");
        return;
    }

    // Soporta multiples rutas pasadas por argumentos
    while (tr[i] != NULL) {
        struct stat st;

        // Comprobamos si la ruta existe y que tipo de elemento es
        if (lstat(tr[i], &st) == -1) {
            perror(tr[i]);
        } else if (S_ISDIR(st.st_mode)) {
            // Si es un directorio, lanzamos el borrado recursivo
            if (borrar_recursivo(tr[i]) == -1) {
                perror(tr[i]);
            }
        } else {
            // Si por error le pasan un fichero normal, lo borramos con unlink
            if (unlink(tr[i]) == -1) {
                perror(tr[i]);
            }
        }
        i++;
    }
}

char LetraTF (mode_t m)
{
     switch (m&S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */ 
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
     }
}

char * ConvierteModo2 (mode_t m)
{
    static char permisos[12];
    strcpy (permisos,"---------- ");
    
    permisos[0]=LetraTF(m);
    if (m&S_IRUSR) permisos[1]='r';    /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';
    if (m&S_IXUSR) permisos[3]='x';
    if (m&S_IRGRP) permisos[4]='r';    /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';
    if (m&S_IXGRP) permisos[6]='x';
    if (m&S_IROTH) permisos[7]='r';    /*resto*/
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';
    if (m&S_ISUID) permisos[3]='s';    /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';
    
    return permisos;
}

void ListFile (char *name, int llong, int link, int acc) {
    struct stat s; // Tipo proporcionado por sys/stat.h para almacenar información sobre el archivo

    // Buffers
    char time[64];
    char link_target[MAXNAME];

    if (lstat(name, &s) == -1) { // lstat para obtener información sobre el archivo, incluyendo enlaces simbólicos
        char err_msg[256];
        sprintf(err_msg, "Error al acceder a %s", name);
        perror(err_msg);
        return;
    }

    if (!llong) { // Si no se requiere formato long, solo imprimimos el tamaño y el nombre del archivo
        printf("%9ld %s\n", (long)s.st_size, name);
        return;
    }

    time_t t = acc ? s.st_atime : s.st_mtime; // si acc, usamos st_atime (último acceso), sino st_mtime (última modificación)
    struct tm *tm_info = localtime(&t);
    strftime(time, sizeof(time), "%Y/%m/%d-%H:%M", tm_info); // Formateo de fecha y hora

    struct passwd *pw = getpwuid(s.st_uid); // Obtener información del usuario propietario del archivo
    struct group *gr = getgrgid(s.st_gid); // Obtener información del grupo propietario del archivo
    char user[32], group[32];

    // Guardamos el nombre del usuario y del grupo, o el UID/GID si no se encuentra
    if (pw) strcpy(user, pw->pw_name); else sprintf(user, "%d", s.st_uid);
    if (gr) strcpy(group, gr->gr_name); else sprintf(group, "%d", s.st_gid);

    link_target[0] = '\0';
    if (link && S_ISLNK(s.st_mode)) { // Si es un enlace simbólico y se requiere mostrar el destino del enlace
        ssize_t len = readlink(name, link_target, sizeof(link_target) - 1); // Lee el destino del enlace simbólico
        if (len != -1) {
            link_target[len] = '\0';
        }
    }

    if (link_target[0] != '\0') { // Si hay un destino de enlace simbólico, lo mostramos junto con la información del archivo
        printf("%s %3lu %8s %8s %s %9ld %s -> %s\n", 
            time, (unsigned long)s.st_nlink, user, group, 
            ConvierteModo2(s.st_mode), (long)s.st_size, name, link_target);
    } else { // Sino, solo mostramos la información del archivo
        printf("%s %3lu %8s %8s %s %9ld %s\n", 
            time, (unsigned long)s.st_nlink, user, group, 
            ConvierteModo2(s.st_mode), (long)s.st_size, name);
    }
}

void Cmd_listfile(char *tr[]) {
    int llong = 0, link = 0, acc = 0;
    int i = 0;

    // Extraer flags de los parámetros, si los hay
    while (tr[i] != NULL && tr[i][0] == '-') {
        if (!strcmp(tr[i], "-long")) llong = 1;
        else if (!strcmp(tr[i], "-link")) link = 1;
        else if (!strcmp(tr[i], "-acc")) acc = 1;
        else {
            // Si empieza por '-' pero no es un flag conocido, asumimos que es un nombre de archivo extraño
            break;
        }
        i++;
    }

    // Si no hay parámetros (o solo hay flags), operamos en directorio actual
    if (tr[i] == NULL) {
        ListFile(".", llong, link, acc);
        return;
    }

    // Si hay parámetros, listamos cada uno de ellos
    while (tr[i] != NULL) {
        ListFile(tr[i], llong, link, acc);
        i++;
    }
}

void ListDir(char *name, int reca, int recb, int hid, int llong, int link, int acc) {
    DIR *d;
    struct dirent *ent;
    struct stat s;
    char path[MAXNAME];

    if (recb) {
        if ((d = opendir(name)) != NULL) { // Abrimos el directorio
            while ((ent = readdir(d)) != NULL) { // Leemos cada entrada del directorio
                if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue; // Ignoramos las entradas "." y ".."
                if (!hid && ent->d_name[0] == '.') continue; // Ignoramos archivos ocultos si no se requiere mostrarlos

                snprintf(path, sizeof(path), "%s/%s", name, ent->d_name); // Construimos la ruta completa del archivo/directorio
                
                if (lstat(path, &s) != -1 && S_ISDIR(s.st_mode)) { // Si es un directorio, llamamos recursivamente a ListDir
                    ListDir(path, reca, recb, hid, llong, link, acc); // Llamada recursiva para listar el contenido del subdirectorio
                }
            }
            closedir(d);
        }
    }

    printf("************ %s ************\n", name);
    if ((d = opendir(name)) != NULL) { // Abrimos el directorio para listar su contenido
        while ((ent = readdir(d)) != NULL) { // Leemos cada entrada del directorio
            if (!hid && ent->d_name[0] == '.') continue; // Ignoramos archivos ocultos si no se requiere mostrarlos

            snprintf(path, sizeof(path), "%s/%s", name, ent->d_name); // Construimos la ruta completa del archivo/directorio

            ListFile(path, llong, link, acc); // Llamamos a ListFile para mostrar la información del archivo/directorio
        }
        closedir(d);
    } else {
        perror("Imposible abrir directorio");
    }

    if (reca) { // Para recursividad después de listar el directorio, abrimos el directorio para procesar sus subdirectorios
        if ((d = opendir(name)) != NULL) { // Abrimos el directorio
            while ((ent = readdir(d)) != NULL) { // Repetimos el proceso de lectura de entradas del directorio
                if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
                if (!hid && ent->d_name[0] == '.') continue;

                snprintf(path, sizeof(path), "%s/%s", name, ent->d_name);
                
                if (lstat(path, &s) != -1 && S_ISDIR(s.st_mode)) {
                    ListDir(path, reca, recb, hid, llong, link, acc);
                }
            }
            closedir(d);
        }
    }
}

void Cmd_list(char *tr[]) {
    // Inicializamos las variables de flags y el índice
    int reca = 0, recb = 0, hid = 0, llong = 0, link = 0, acc = 0;
    int i = 0;
    struct stat s;

    while (tr[i] != NULL && tr[i][0] == '-') { // Procesamos los flags
        if (!strcmp(tr[i], "-reca")) reca = 1;
        else if (!strcmp(tr[i], "-recb")) recb = 1;
        else if (!strcmp(tr[i], "-hid")) hid = 1;
        else if (!strcmp(tr[i], "-long")) llong = 1;
        else if (!strcmp(tr[i], "-link")) link = 1;
        else if (!strcmp(tr[i], "-acc")) acc = 1;
        else break;
        i++;
    }

    if (tr[i] == NULL) { // Si no hay parámetros, listamos el directorio actual
        ListDir(".", reca, recb, hid, llong, link, acc);
        return;
    }

    while (tr[i] != NULL) { // Listamos cada parámetro proporcionado
        if (lstat(tr[i], &s) == -1) {
            perror(tr[i]);
        } else {
            if (S_ISDIR(s.st_mode)) { // Si es un directorio, llamamos a ListDir para listar su contenido
                ListDir(tr[i], reca, recb, hid, llong, link, acc);
            } else {
                ListFile(tr[i], llong, link, acc);
            }
        }
        i++;
    }
}