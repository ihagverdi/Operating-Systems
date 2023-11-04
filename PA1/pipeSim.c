/*
Author: Hagverdi Ibrahimli
Date:   04/11/2023
Description ' Simulating the pipe command in Linux. 
              Specifically, we are simulating man ping | grep -A4 -- -A > bash.txt command.
            '
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    printf("I'm SHELL process, with PID: %d - Main command is: <man ping | grep -A4 -- -A > output.txt>\n", (int) getpid());
    
    int file_dsc[2];
    
    int success = pipe(file_dsc); //create pipe
    assert(success != -1);

    int rc1 = fork(); //first fork for man command
    
    if (rc1 < 0) {
		fprintf(stderr, "fork failed\n");
		exit(1);
	}
    else if (rc1 == 0) {
        //child man process
        printf("I’m MAN process, with PID: %d - My command is: <man ping>\n", (int) getpid());
        close(file_dsc[0]); //close read end of pipe
        dup2(file_dsc[1], STDOUT_FILENO); //redirect stdout to write end of pipe
        close(file_dsc[1]); //close write end of pipe
        char* argument_list[] = {"man",  "ping", NULL};
        execvp(argument_list[0], argument_list);
        printf("execvp for man failed\n");
    }

    int rc2 = fork(); //second fork by parent for grep process
    
    if (rc2 < 0) {
    fprintf(stderr, "fork failed\n");
    exit(1);
    }

    else if (rc2 == 0) {
        //child grep process
        printf("I’m GREP process, with PID: %d - My command is: <grep -A4 -- -A>\n", (int) getpid());
        close(file_dsc[1]); //close write end of pipe
        dup2(file_dsc[0], STDIN_FILENO); //redirect stdin to read end of pipe
        close(file_dsc[0]); //close read end of pipe
        FILE* fp = fopen("output.txt", "w"); //create the output file
        if (fp == NULL) {
            printf("Error: Failed to create output file\n");
            exit(1);
        }
        dup2(fileno(fp), STDOUT_FILENO); //redirect stdout to output file
        char* argument_list[] = {"grep", "-A4", "--", "-A", NULL};
        execvp(argument_list[0], argument_list);
        printf("execvp for grep failed\n");

    }
    
    close(file_dsc[0]);
    close(file_dsc[1]);

    waitpid(rc1, NULL, 0);
    waitpid(rc2, NULL, 0);

    printf("I’m SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", (int) getpid());

    return 0;
}