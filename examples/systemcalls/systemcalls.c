#include "systemcalls.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define NO_ERROR 0
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    int8_t system_return=system(cmd);

    if(NO_ERROR!=system_return)
    {
        return false;
    }

    if ((WEXITSTATUS(system_return) == 0) && WIFEXITED(system_return) )
    {
        printf("\nsystem() terminated gracefully\n");
        return true;
    }


    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    bool returnVlaue = false;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    pid_t pid = fork();

    // execv()
    if(pid < 0)
    {
        returnVlaue = false;
    }
    else if(pid == 0)
    {
        // Child process
        execv(command[0], command);
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        int status;
        wait(&status);

        if(WIFEXITED(status) && (EXIT_FAILURE!=WEXITSTATUS(status)))
        {
                // Child terminated normally.
                returnVlaue = true;
        }
        else
        {
            // Child terminated abnormally.
            returnVlaue = false;
        }
    }

    va_end(args);

    return returnVlaue;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

 int status;


    pid_t pid;


    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC , 0644);

    if(fd == -1){

        exit(EXIT_FAILURE);
    }
    
    pid=fork();

    if(pid == -1){
        return false;
    }

   
     /*child Process*/
    if(pid == 0){
    
    if(dup2(fd,STDOUT_FILENO) == -1){
        close(fd);
        exit(EXIT_FAILURE);
    }

        close(fd);
        execv(command[0],command);
        _exit(EXIT_FAILURE);
    }

    else if(pid > 0){
        // int status;

        if (waitpid(pid,&status,0) == -1){
            return false;
        }

    }


    va_end(args);

    return WIFEXITED(status) && (WEXITSTATUS(status) == 0) ;
}
