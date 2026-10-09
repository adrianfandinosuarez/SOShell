#ifndef ARQUIVOS_H
#define ARQUIVOS_H
#define MAXNAME 1024

typedef struct {
    int df;
    int mode;
    char name[MAXNAME];
} FileEntry;

void Cmd_trychdir(char *args[]);
void InitializeOpenFiles();
void Cmd_open (char * tr[]);
void Cmd_close (char * tr[]);
void Cmd_listopen(char * tr[]);
void Cmd_dup(char * tr[]);
void Cmd_lseek(char * tr[]);
void Cmd_readstr(char * tr[]);
void Cmd_writestr(char * tr[]);
void Cmd_makefile(char * tr[]);
void Cmd_makedir(char * tr[]);
void Cmd_delete(char * tr[]);
void Cmd_deltree(char * tr[]);
void Cmd_listfile(char * tr[]);
void Cmd_list(char * tr[]);
#endif