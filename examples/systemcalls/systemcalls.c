#include "systemcalls.h"
#include <stdlib.h> // For system, exit
#include <unistd.h> // For fork, execv, close, dup2, STDOUT_FILENO
#include <sys/wait.h> // For waitpid
#include <fcntl.h> // For open
#include <stdio.h> // For fflush, perror

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 * successfully using the system() call, false if an error occurred,
 * either in invocation of the system() call, or if a non-zero return
 * value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    /*
     * TODO  add your code here
     * Call the system() function with the command set in the cmd
     * and return a boolean true if the system() call completed with success
     * or false() if it returned a failure
    */
    int ret = system(cmd);

    // system() returns -1 on error, or the exit status of the command otherwise.
    // WIFEXITED checks if the child terminated normally.
    // WEXITSTATUS gets the exit status of the child.
    if (ret == -1) {
        perror("system() call failed");
        return false;
    } else if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        return true; // Command executed successfully and returned 0
    } else {
        // Command executed but returned a non-zero exit code, or terminated abnormally.
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
* followed by arguments to pass to the command
* Since exec() does not perform path expansion, the command to execute needs
* to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
* The first is always the full path to the command to execute with execv()
* The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
* using the execv() call, false if an error occurred, either in invocation of the
* fork, waitpid, or execv() command, or if a non-zero return value was returned
* by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count + 1]; // +1 for NULL terminator
    int i;
    for(i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL; // execv requires a NULL-terminated array of arguments

    /*
     * TODO:
     * Execute a system command by calling fork, execv(),
     * and wait instead of system (see LSP page 161).
     * Use the command[0] as the full path to the command to execute
     * (first argument to execv), and use the remaining arguments
     * as second argument to the execv() command.
     *
    */
    
    // Ensure stdout buffer is flushed before forking to avoid duplicated output
    fflush(stdout); 

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        perror("fork() failed");
        va_end(args);
        return false;
    } else if (pid == 0) {
        // Child process
        execv(command[0], command);
        // If execv returns, it means an error occurred
        perror("execv() failed in child");
        exit(1); // Exit with an error code if execv fails
    } else {
        // Parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            // waitpid failed
            perror("waitpid() failed");
            va_end(args);
            return false;
        }

        // Check the child's exit status
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Child exited normally with status 0 (success)
            va_end(args);
            return true;
        } else {
            // Child exited with non-zero status or abnormally
            // For example, if WIFSIGNALED(status) is true, it was terminated by a signal.
            va_end(args);
            return false;
        }
    }
}

/**
* @param outputfile - The full path to the file to write with command output.
* This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count + 1]; // +1 for NULL terminator
    int i;
    for(i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL; // execv requires a NULL-terminated array of arguments


    /*
     * TODO
     * Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
     * redirect standard out to a file specified by outputfile.
     * The rest of the behaviour is same as do_exec()
     *
    */

    // Ensure stdout buffer is flushed before forking to avoid duplicated output
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        perror("fork() failed");
        va_end(args);
        return false;
    } else if (pid == 0) {
        // Child process
        // Open the output file for writing, create if not exists, truncate if exists
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Failed to open output file in child process");
            exit(1); // Exit with an error code
        }

        // Redirect standard output (STDOUT_FILENO) to the opened file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout in child process");
            close(fd); // Close the file descriptor before exiting
            exit(1); // Exit with an error code
        }
        close(fd); // Close the original file descriptor as stdout is now redirected

        execv(command[0], command);
        // If execv returns, it means an error occurred
        perror("execv() failed in child (after redirection)");
        exit(1); // Exit with an error code if execv fails
    } else {
        // Parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            // waitpid failed
            perror("waitpid() failed");
            va_end(args);
            return false;
        }

        // Check the child's exit status
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Child exited normally with status 0 (success)
            va_end(args);
            return true;
        } else {
            // Child exited with non-zero status or abnormally
            va_end(args);
            return false;
        }
    }
}