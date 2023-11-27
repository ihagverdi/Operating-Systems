/*
CLI simulator
Author: Hagverdi Ibrahimli 30014
Date: 27/11/2023
*/
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <fstream>
#include <sys/wait.h>
#include <vector>
using namespace std;

//initialize the global mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//initialize the vectors to do bookkeeping for wait
vector<pthread_t> threads;
vector<pid_t> pids;


//helper functions
void* listener(void*);
void WaitForPT();

//starting point of the program
int main() {
    //check if the file exists
    ifstream inFile("commands.txt");
    if (!inFile.is_open()) {
        cout << "Error opening file" << endl;
        return 1;
    }

    //shell process starts the parsing of the "commands.txt"
    ofstream outFile("parse.txt");
    if (!outFile.is_open()) {
        cout << "Error opening file" << endl;
        return 1;
    }
    
    string line;
    while (getline(inFile, line)) {
        //initialize variables for each command line
        string command, input, option, redirection_fileName = "";
        bool background_process = false;
        char redirection_sign = '-';

        char* args[4] = {NULL, NULL, NULL, NULL}; //to pass to execvp

        stringstream ss(line);
        string word;
        ss >> word;
        //first word must contain the command
        command = word;
        args[0] = strdup(command.c_str());
        int i = 1;
        //fill the rest of the args array
        //check if there is any other word in the command line
        while (ss >> word) {
            if (word == "&") { //check if it is a background process
                background_process = true;
            }
            else if (word == "<" || word == ">") { //check if it is a redirection
                redirection_sign = word[0];
                //get the next word as the file name
                ss >> word;
                redirection_fileName = word;
            }

            else if (word[0] == '-') { //check if it is an option
                option = word;
                args[i] = strdup(option.c_str());
                i++;
            }

            else { //must be an input
                input = word;
                args[i] = strdup(input.c_str());
                i++;
            }
            
        }
        //write command details to parse.txt
        outFile << "----------" << endl;
        outFile << "Command: " << command << endl;
        outFile << "Inputs: " << input << endl;
        outFile << "Options: " << option << endl;
        outFile << "Redirection: " << redirection_sign << endl;
        outFile << "Background Job: " << ((background_process) ? "y" : "n") << endl;
        outFile << "----------" << endl;
        outFile.flush();
        
        if (command == "wait") {
            // wait for all backround processes and threads to finish
            WaitForPT();
        }

        else if (redirection_sign == '>') {
            //output is to a file
            pid_t redirect_pid = fork();
            if (redirect_pid < 0) {
                cout << "Error creating child process" << endl;
                return 1;
            }
            if (redirect_pid == 0) {
                //child process
                int fd = open(redirection_fileName.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
                dup2(fd, STDOUT_FILENO);
                close(fd); //close the unused one
                execvp(args[0], args);
            }
            else if (redirect_pid > 0) {
                //parent process
                if (background_process) {
                    //background process
                    pids.push_back(redirect_pid);
                }
                else {
                    //foreground process
                    waitpid(redirect_pid, NULL, 0);
                }
            }
        }
        else {
            //output is to the terminal
            int myPipe[2];
            pipe(myPipe);
            pthread_t thread;

            pid_t myPid = fork();

            if (myPid < 0) {
                cout << "Error creating child process" << endl;
                return 1;
            }
            else if (myPid == 0) {
                //child process
                if (redirection_sign == '<') {
                    //input is from a file
                    int fd2 = open(redirection_fileName.c_str(), O_RDONLY);
                    dup2(fd2, STDIN_FILENO);
                    close(fd2); //close the unused one
                }
                //redirect the output to the pipe
                dup2(myPipe[1], STDOUT_FILENO);
                close(myPipe[0]); //close the unused one
                close(myPipe[1]); //close the unused one
                execvp(args[0], args);
            } 
            else if (myPid > 0) {
                //parent process
                close(myPipe[1]); //close the unused one
                pthread_create(&thread, NULL, listener, (void*) myPipe[0]);
                if (background_process) {
                    //background process
                    pids.push_back(myPid);
                    threads.push_back(thread);
                }
                else {
                    //foreground process
                    waitpid(myPid, NULL, 0);
                    pthread_join(thread, NULL);
                }
            }
        }
        
    }
    //executed all commands
    //close files
    
    outFile.close();
    inFile.close();
    
    //wait for all backround processes and threads to finish
    WaitForPT();
    return 0;
}

void* listener(void* arg) {
    //read and print from the pipe until it is closed
    long int fd = (long int)arg;
    bool still_trying = true;
    char buffer[1024];
    FILE* file = fdopen(fd, "r");
    while (still_trying) {
        pthread_mutex_lock(&mutex); //get the lock first (according to pdf description)
        if (fgets(buffer, sizeof(buffer), file) != NULL) { //able to read, print the first line
            printf("---- %ld\n", pthread_self());
            printf("%s", buffer);
            fsync(STDOUT_FILENO);
            while (fgets(buffer, sizeof(buffer), file) != NULL) { //read until the end of the file
                printf("%s", buffer);
                fsync(STDOUT_FILENO);
            }
            printf("---- %ld\n", pthread_self());
            fsync(STDOUT_FILENO);
            still_trying = false;
        } //if file is not opened or read is not successful, try again
        pthread_mutex_unlock(&mutex); //release the lock
    }
    fclose(file);
    return NULL;
}

void WaitForPT() {
    //wait for all background processes and threads to finish
    for (int i = 0; i < pids.size(); i++) {
        waitpid(pids[i], NULL, 0);
    }

    pids.clear();

    for (int i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
    }

    threads.clear();
}