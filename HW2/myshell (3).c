#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

//process_arglist implementation starts at line 274

//functions declarations
int execute_foreground(char **arglist);
int execute_background(char **arglist);
int execute_single_pipe(char **arglist, int i);
int execute_output_redirecting(char **arglist, int i);

struct sigaction ignore;

//prepare function
int prepare(void)
{
    //register to ignoring handler of SIGINT
    struct sigaction new_action;
    memset(&new_action,0,sizeof(new_action));
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;
    new_action.sa_handler = SIG_IGN;
    if(sigaction(SIGINT, &new_action, NULL) != 0){
        perror("failed registration to the SIGINT handler");
        exit(1);
    }

    //register to SIGCHLD "handler"-ignore
    memset(&ignore,0,sizeof(ignore));
    ignore.sa_flags= SA_RESTART | SA_NOCLDSTOP | SA_NOCLDWAIT;
    ignore.sa_handler = SIG_IGN;
    if(sigaction(SIGCHLD, &ignore, NULL) != 0){
        perror("failed registration to the SIGCHLD handler");
        exit(1);
    }

    return 0;
}

int finalize(void)
{
    return 0;
}

int execute_foreground(char **arglist)
{
    int pid = fork();

    if (pid == -1) {
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }

    if(pid == 0){ //in child
        //registration to SIGINT default handler
        struct sigaction new_action;
        memset(&new_action,0,sizeof(new_action));
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;
        new_action.sa_handler = SIG_DFL;
        if(sigaction(SIGINT, &new_action, NULL) != 0){
            perror("failed registration to the SIGINT handler");
            exit(1);
        }

        if (execvp(arglist[0], arglist)== -1) {
            perror("Failed execvp");
            exit(EXIT_FAILURE);
        }
    }

    int waiting_value = waitpid(pid, NULL, 0);

    if(waiting_value == -1){
        if(errno != ECHILD && errno != EINTR){
            perror("Failed waiting");
            return 0;
        }
    }

    return 1;
}

int execute_background(char **arglist)
{
    int pid = fork();
    if (pid == -1) {
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }

    if(pid == 0){
        //registration to SIGINT ignoring handler
        struct sigaction new_action;
        memset(&new_action,0,sizeof(new_action));
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;
        new_action.sa_handler = SIG_IGN;
        if(sigaction(SIGINT, &new_action, NULL) != 0){
            perror("failed registration to the SIGINT handler");
            exit(1);
        }

        if (execvp(arglist[0], arglist)== -1) {
            perror("Failed executing aux");
            exit(EXIT_FAILURE);
        }
    }

    //no waiting her
    return 1;
}

int execute_single_pipe(char **arglist, int i){
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Failed creating pipe");
        exit(EXIT_FAILURE);
    }

    int pid1 = fork();

    if(pid1 == -1){
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }

    if(pid1 == 0){
        //registration to SIGINT default handler
        struct sigaction new_action;
        memset(&new_action,0,sizeof(new_action));
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;
        new_action.sa_handler = SIG_DFL;
        if(sigaction(SIGINT, &new_action, NULL) != 0){
            perror("failed registration to the SIGINT handler");
            exit(1);
        }

        //left command process
        if(dup2(fd[1], STDOUT_FILENO) == -1){
            perror("Failed dupping");
            exit(EXIT_FAILURE);
        }

        close(fd[0]);
        close(fd[1]);
        if (execvp(arglist[0], arglist)== -1) {
            perror("Failed execvp");
            exit(EXIT_FAILURE);
        }
    }

    int pid2 = fork();

    if(pid2 == -1){
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }

    if(pid2 == 0){
        //registration to SIGINT default handler
        struct sigaction new_action;
        memset(&new_action,0,sizeof(new_action));
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;
        new_action.sa_handler = SIG_DFL;
        if(sigaction(SIGINT, &new_action, NULL) != 0){
            perror("failed registration to the SIGINT handler");
            exit(1);
        }

        //right command process
        if(dup2(fd[0], STDIN_FILENO) == -1){
            perror("Failed dupping");
            exit(EXIT_FAILURE);
        }

        close(fd[0]);
        close(fd[1]);
        if (execvp(arglist[i + 1], arglist + i + 1) == -1) { //need the +1 her?
            perror("Failed execvp");
            exit(EXIT_FAILURE);
        }
    }

    close(fd[0]);
    close(fd[1]);

    int waiting_value1 = waitpid(pid1, NULL, 0);

    if(waiting_value1 == -1){
        if(errno != ECHILD && errno != EINTR){
            perror("Failed waiting");
            return 0;
        }
    }

    int waiting_value2 = waitpid(pid2, NULL, 0);

    if(waiting_value2 == -1){
        if(errno != ECHILD && errno != EINTR){
            perror("Failed waiting");
            return 0;
        }
    }

    return 1;
}

int execute_output_redirecting(char **arglist, int i){
    int pid = fork();

    if (pid == -1) {
        perror("failed forking");
        exit(EXIT_FAILURE);
    }

    if(pid == 0) {
        //registration to SIGINT default handler
        struct sigaction new_action;
        memset(&new_action,0,sizeof(new_action));
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;
        new_action.sa_handler = SIG_DFL;
        if(sigaction(SIGINT, &new_action, NULL) != 0){
            perror("failed registration to the SIGINT handler");
            exit(1);
        }

        char* file_name = arglist[i + 1];
        int file = open(file_name, O_WRONLY | O_CREAT, 0777);
        if(file == -1){
            perror("Failed opening file"); //do we need to close the files?
            close(file);
            exit(EXIT_FAILURE);
        }

        if(dup2(file, STDOUT_FILENO) == -1){
            perror("Failed dupping");
            exit(EXIT_FAILURE);
        }

        close(file);

        if (execvp(arglist[0], arglist) == -1) {
            perror("Failed executing aux");
            exit(EXIT_FAILURE);
        }
    }

    int waiting_value =waitpid(pid, NULL, 0);

    if(waiting_value == -1){
        if(errno != ECHILD &&errno != EINTR){
            perror("Failed waiting");
            return 0;
        }
    }

    return 1;

}

int process_arglist(int count,char **arglist)
{
    int i = 0;
    int flag = 1; //flag that indicates what is the type of the command
    int num = 0; //number of special strings (s.a "|" or ">")

    while(arglist[i] != NULL)
    {

        if(arglist[i] != NULL && strcmp(arglist[i], "&") == 0) //background process
        {
            flag = 2;
            arglist[i] = NULL;
        }

        if(arglist[i] != NULL && strcmp(arglist[i], "|") == 0) //single piping
        {
            flag = 3;
            arglist[i] = NULL;
            num = i;
        }

        if(arglist[i] != NULL && strcmp(arglist[i], ">") == 0) //output redirecting
        {
            flag = 4;
            arglist[i] = NULL;
            num = i;
        }

        i++;
    }

    if(flag == 1){
        return execute_foreground(arglist); //foreground command
    }

    if(flag == 2){
        return execute_background(arglist); //background command
    }

    if(flag == 3){
        return execute_single_pipe(arglist, num); //single pipe command
    }

    if(flag == 4){
        return execute_output_redirecting(arglist, num); //output redirecting command
    }

    //shouldn't get her
    return 1;
}




