#include "systemcalls.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    return (system(cmd) == 0 ? true : false);
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
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    // Create a child process
    fflush(stdout);
    pid_t pid = fork();

    if (pid == -1) {
      perror("fork");
      va_end(args);
      return false;
    }


    if (pid == 0) {
        // Child process
        execv(command[0], command);
        perror("execv"); // Execv will only return if an error occurs
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        wait(&status); // Wait for the child process to finish

        va_end(args);

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                fprintf(stderr, "Child process exited with non-zero status: %d\n", exit_status);
                return false;
            }
        } else {
            fprintf(stderr, "Child process did not terminate normally.\n");
            return false;
        }
    }

    va_end(args);

    return true;
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

    if (outputfile == NULL){
      return false;
    }

    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
        int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
        if (dup2(fd, 1) < 0) {perror("dup2"); abort(); }
        close(fd);
        execvp(command[0], command);
        perror("execv");  // Execv will only return if an error occurs
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        wait(&status);  // Wait for the child process to finish

        va_end(args);

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                fprintf(stderr, "Child process exited with non-zero status: %d\n", exit_status);
                return false;
            }
        } else {
            fprintf(stderr, "Child process did not terminate normally.\n");
            return false;
        }
    }

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

    return true;
}
