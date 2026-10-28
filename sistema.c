#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include "sistema.h"

void Cmd_date (char *arg[]) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char buffer[26];

    if (arg[0] == NULL) {
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("Fecha y hora actual: %s\n", buffer);
    } else if (strcmp(arg[0], "-d") == 0) {
        strftime(buffer, 26, "%Y-%m-%d", tm_info);
        printf("Fecha actual: %s\n", buffer);
    } else if (strcmp(arg[0], "-t") == 0) {
        strftime(buffer, 26, "%H:%M:%S", tm_info);
        printf("Hora actual: %s\n", buffer);
    } else {
        printf("Uso: date [-d] [-t]\n");
    }
}

void Cmd_sysinfo (char *arg[]) {
    struct utsname sysinfo;
    if (uname(&sysinfo) == -1) {
        perror("Imposible obtener información del sistema");
        return;
    }

    printf("Sistema operativo: %s\n", sysinfo.sysname);
    printf("Nombre del nodo: %s\n", sysinfo.nodename);
    printf("Versión del kernel: %s\n", sysinfo.release);
    printf("Versión del sistema: %s\n", sysinfo.version);
    printf("Arquitectura de la máquina: %s\n", sysinfo.machine);
}

void Cmd_help(char *tr[]) {
    if (tr[0] == NULL) {
        printf("Comandos disponibles:\n");
        printf("fin exit quit bye date pid pwd chdir autores authors exec pplano splano path importpath where trychdir sysinfo help open close listopen dup lseek readstr writestr makefile makedir delete listfile\n");
        return;
    }

    if (!strcmp(tr[0], "fin") || !strcmp(tr[0], "exit") || !strcmp(tr[0], "quit") || !strcmp(tr[0], "bye")) {
        printf("%s\t\t\tTermina la ejecucion del shell\n", tr[0]);
    } else if (!strcmp(tr[0], "date")) {
        printf("date [-d|-t]\t\tMuestra la fecha y/o la hora actual\n");
    } else if (!strcmp(tr[0], "pid")) {
        printf("pid [-p]\t\tMuestra el PID del shell o de su proceso padre\n");
    } else if (!strcmp(tr[0], "pwd")) {
        printf("pwd\t\t\tMuestra el directorio actual de trabajo\n");
    } else if (!strcmp(tr[0], "chdir")) {
        printf("chdir [dir]\t\tCambia el directorio actual o lo muestra si no hay parametros\n");
    } else if (!strcmp(tr[0], "autores") || !strcmp(tr[0], "authors")) {
        printf("%s [-l|-n]\t\tMuestra los nombres y/o logins de los autores\n", tr[0]);
    } else if (!strcmp(tr[0], "exec")) {
        printf("exec prog [args...]\tEjecuta un programa sin crear proceso, reemplazando el shell\n");
    } else if (!strcmp(tr[0], "pplano")) {
        printf("pplano prog [args...]\tEjecuta un programa en primer plano (crea proceso hijo)\n");
    } else if (!strcmp(tr[0], "splano")) {
        printf("splano prog [args...]\tEjecuta un programa en segundo plano (background)\n");
    } else if (!strcmp(tr[0], "path")) {
        printf("path [-add|-del|-show|-clear|-import] [dir]\n");
        printf("\t\t\tGestiona la lista de directorios de busqueda de ejecutables\n");
    } else if (!strcmp(tr[0], "importpath")) {
        printf("importpath\t\tImporta el PATH del entorno al path interno del shell\n");
    } else if (!strcmp(tr[0], "where")) {
        printf("where prog\t\tMuestra la ruta completa del ejecutable buscandolo en el path\n");
    } else if (!strcmp(tr[0], "trychdir")) {
        printf("trychdir [-s] dir...\tIntenta cambiar a varios directorios. -s para modo silencioso\n");
    } else if (!strcmp(tr[0], "sysinfo")) {
        printf("sysinfo\t\t\tMuestra informacion del sistema operativo y la maquina\n");
    } else if (!strcmp(tr[0], "help")) {
        printf("help [cmd]\t\tMuestra ayuda sobre los comandos disponibles\n");
    } else if (!strcmp(tr[0], "open")) {
        printf("open file m1 m2...\tAbre el fichero file.\n");
        printf("\t\t\tModos: cr, ap, ex, ro, rw, wo, tr\n");
    } else if (!strcmp(tr[0], "close")) {
        printf("close df [-f]\t\tCierra el descriptor de fichero df\n");
    } else if (!strcmp(tr[0], "listopen")) {
        printf("listopen\t\tLista los ficheros actualmente abiertos por el shell\n");
    } else if (!strcmp(tr[0], "dup")) {
        printf("dup df\t\t\tDuplica el descriptor de fichero df\n");
    } else if (!strcmp(tr[0], "lseek")) {
        printf("lseek df pos ref\tPosiciona el offset del fichero.\n");
        printf("\t\t\tref: SEEK_SET, SEEK_CUR, SEEK_END\n");
    } else if (!strcmp(tr[0], "readstr")) {
        printf("readstr df cont\t\tLee cont bytes del fichero con descriptor df\n");
        printf("\t\t\ty los muestra en pantalla\n");
    } else if (!strcmp(tr[0], "writestr")) {
        printf("writestr df string\tEscribe la cadena en el fichero descrito por df\n");
    } else if (!strcmp(tr[0], "makefile")) {
        printf("makefile name\t\tCrea un fichero vacio de nombre name\n");
    } else if (!strcmp(tr[0], "makedir")) {
        printf("makedir name\t\tCrea un directorio de nombre name\n");
    } else if (!strcmp(tr[0], "delete")) {
        printf("delete name1 name2...\tBorra ficheros o directorios (si estan vacios)\n");
    } else if (!strcmp(tr[0], "listfile")) {
        printf("listfile [-long][-link][-acc] name1 name2...\n");
        printf("\t\t\tLista informacion detallada de ficheros;\n");
        printf("\t\t\t-long: listado largo\n");
        printf("\t\t\t-acc:  tiempo de acceso\n");
        printf("\t\t\t-link: si es enlace simbolico, muestra a donde apunta\n");
    } else if (!strcmp(tr[0], "list")) {
        printf("list [-reca] [-recb] [-hid][-long][-link][-acc] name1 name2...\n");
        printf("\t\t\tIgual que listfile, pero si el nombre es un directorio,\n");
        printf("\t\t\tlista sus contenidos en lugar del propio directorio.\n");
        printf("\t\t\t-hid:  incluye ficheros ocultos\n");
        printf("\t\t\t-reca: recursividad DESPUES de listar el directorio\n");
        printf("\t\t\t-recb: recursividad ANTES de listar el directorio\n");
    } else {
        printf("Comando no encontrado. Teclee 'help' para ver la lista de comandos disponibles.\n");
    }
}