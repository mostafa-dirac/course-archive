#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
    printf("Bye\n");
    exit(0);
    return 1;
}

int cmd_help(tok_t arg[]);
int cmd_pwd(tok_t arg[]);
int cmd_cd(tok_t arg[]);
int cmd_wait(tok_t arg[]);
int dirfunc(tok_t arg[]);
int genfunc(tok_t arg[]);
int file_redirect(tok_t arg[]);
void enable_signal(void);
void setctrl(void);
int is_background(tok_t * arg);


/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
    cmd_fun_t *fun;
    char *cmd;
    char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
        {cmd_help, "?", "show this help menu"},
        {cmd_quit, "quit", "Quit the command shell"},
        {cmd_pwd, "pwd", "Get current working directory."},
        {cmd_cd, "cd", "Change the shell working directory."},
        {cmd_wait, "wait", "Waits for all background processes to terminate"}
};

int cmd_help(tok_t arg[]) {
    int i;
    for (i = 0; i < (sizeof(cmd_table) / sizeof(fun_desc_t)); i++) {
        printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
    }
    return 1;
}

int cmd_pwd(tok_t arg[]){
    char directory[100];
    if( getcwd(directory, sizeof(directory)) != NULL){
        printf("%s\n", directory);
        return 0;
    } else{
        fprintf(stderr,"Can not get current working directory.\n");
    }
    return 1;
}

int cmd_cd(tok_t arg[]){
    if( arg[1] != NULL ){
        fprintf(stderr,"Too many arguments.\n");
        return 1;
    }
    if( chdir(arg[0]) != 0 ){
        fprintf(stderr,"No such file or directory.\n");
        return 1;
    } else
        return 0;
}

int cmd_wait(tok_t arg[]){
    int status;
    while(wait(&status)){
        //wait
    }
    return 1;
}

int dirfunc(tok_t *arg){
    int bg = is_background(arg);
    int status = 0;
    int sta = 0;
    int cpid = fork();
    if(cpid > 0){
        if (!bg) {
            waitpid(cpid, &sta, WUNTRACED);
        }
        setctrl();
        return -1 * (sta != 0);
    }
    else if(cpid == 0){
        setpgid(getpid(), getpid());
        if (!bg){
            setctrl();
            enable_signal();
        }
        file_redirect(arg);
        status = execv(arg[0],arg);
        if(status != 0){
            fprintf(stderr, "No such file or directory.\n");
        }
        exit(0);
    }
    else{
        fprintf(stderr, "Please try again.\n");
        exit(-1);
    }

}


int genfunc(tok_t arg[]){
    int bg = is_background(arg);
    int stat = 0;
    int cpid = fork();
    if(cpid > 0){
        if (!bg){
            waitpid(cpid, &stat, WUNTRACED);
        }
        setctrl();
        return -1 * (stat != 0);
    }
    else if(cpid == 0){
        setpgid(getpid(), getpid());
        if (!bg){
            setctrl();
            enable_signal();
        }
        file_redirect(arg);
        tok_t *env;
        env = getToks(getenv("PATH"));
        char adr[1024];
        char* argv[1024] = {adr, NULL};
        int i = 1;
        while (arg[i] != NULL){
            argv[i] = arg[i];
            i++;
        }
        i = 0;
        while (i < MAXTOKS - 1 && env[i] != NULL){
            strcpy(adr, env[i]);
            strcat(adr, "/");
            strcat(adr, arg[0]);
            if (access(adr,X_OK)== 0){
                execv(adr,argv);
                freeToks(env);
                exit(0);
            }
            i++;
        }
        fprintf(stderr, "command not found.\n");
        exit(-1);
    } else{
        fprintf(stderr, "Please try again.\n");
        exit(-1);
    }

}

void enable_signal(void){
    signal(SIGQUIT, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGKILL, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCONT, SIG_DFL);
}

void setctrl(void){
    tcsetpgrp(STDIN_FILENO, getpgid(getpid()));
    tcsetpgrp(STDOUT_FILENO, getpgid(getpid()));
    tcsetpgrp(STDERR_FILENO, getpgid(getpid()));
}

int file_redirect(tok_t arg[]){
    int in_file = isDirectTok(arg, "<");
    int out_file = isDirectTok(arg, ">");
    int file;
    if (in_file != -1){
        file = open(arg[in_file + 1], O_RDONLY, 0);
        dup2(file, STDIN_FILENO);
        int i = in_file;
        arg[i] = NULL;
        arg[i+ 1] = NULL;
        close(file);
    }
    if (out_file != -1){
        file = open(arg[out_file + 1], O_WRONLY |O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH);

        dup2(file, STDOUT_FILENO);
        int i = out_file;
        arg[i] = NULL;
        arg[i+ 1] = NULL;
        close(file);
    }
    return 1;
}


int is_background(tok_t * arg){
    int ind = isDirectTok(arg, "&");
    if (ind > 0){
        arg[ind] = NULL;
        return 1;
    }
    return 0;
}


int lookup(char cmd[]) {
    int i;
    for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
        if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
    }
    return -1;
}

void init_shell()
{
    /* Check if we are running interactively */
    shell_terminal = STDIN_FILENO;

    /** Note that we cannot take control of the terminal if the shell
        is not interactive */
    shell_is_interactive = isatty(shell_terminal);

    if(shell_is_interactive){

        /* force into foreground */
        while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
            kill( - shell_pgid, SIGTTIN);

        shell_pgid = getpid();
        /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

        /* Take control of the terminal */
        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_tmodes);
    }
    /** IS YOUR CODE HERE? */
    /** YES. MY CODE IS HERE */
    signal(SIGQUIT, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
    /** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
    /** YOUR CODE HERE */
    return NULL;
}



int shell (int argc, char *argv[]) {
    char *s = malloc(INPUT_STRING_SIZE + 1);            /* user input string */
    tok_t *t;            /* tokens parsed from input */
    int lineNum = 0;
    int fundex = -1;
    pid_t pid = getpid();        /* get current processes PID */
    pid_t ppid = getppid();    /* get parents PID */
    pid_t cpid, tcpid, cpgid;

    init_shell();

    //printf("%s running as PID %d under %d\n",argv[0],pid,ppid);

    lineNum = 0;
    while ((s = freadln(stdin))) {
        t = getToks(s); /* break the line into tokens */
        fundex = lookup(t[0]); /* Is first token a shell literal */
        if (fundex >= 0) {
            cmd_table[fundex].fun(&t[1]);
        } else {
            if (t[0] != NULL) {
                if (t[0][0] == '/' || t[0][0] == '.')
                    dirfunc(&t[0]);
                else {
                    genfunc(&t[0]);
                }
            }
        }


    }
    return 0;
}
