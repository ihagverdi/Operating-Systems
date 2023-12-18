/*
Author: Hagverdi Ibrahimli
Date: December 18th, 2023
*/
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
using namespace std;

vector<pthread_t> fanThreads;
pthread_barrier_t barrier;
sem_t checkMutex, printMutex;
sem_t semWaitingFansA, semWaitingFansB;
int waitingFansA = 0, waitingFansB = 0;
int carID = -1; //set to -1 initially

/*
Errors:
return 1: Invalid number of arguments
return 2: Invalid number of fans
*/
void* share_ride(void* fanT); //single function for both types of threads: Thread A and Thread B

int main(int argc, char** argv) {
    //check for valid inputs
    if (argc != 3) 
    {
        cout << "The main terminates" << endl;
        return 1;
    }

    int numberOfFansA = atoi(argv[1]);
    int numberOfFansB = atoi(argv[2]);
    int totalFans = numberOfFansA + numberOfFansB;

    //check for negative or odd number of fans; check for total number of fans not divisible by 4
    if ((numberOfFansA < 0 || numberOfFansB < 0) || (numberOfFansA % 2 != 0 ||numberOfFansB % 2 != 0) || (totalFans % 4 != 0))
    {
        cout << "The main terminates" << endl;
        return 2;
    }

    /*valid arguments*/
    //initialize semaphores
    sem_init(&checkMutex, 0, 1); //binary semaphore -> lock for checking the ride status
    sem_init(&printMutex, 0, 1); //binary semaphore -> lock for printing to the console
    //waiting fans semaphore: 0 so that all waiting until a driver (last person to form a 'group') arrives to wake them up
    sem_init(&semWaitingFansA, 0, 0);
    sem_init(&semWaitingFansB, 0, 0);
    //create the first barrier to be used by the first ride
    pthread_barrier_init(&barrier, NULL, 4);


    //create fan threads
    char* fanA = (char*)malloc(sizeof(char));
    char* fanB = (char*)malloc(sizeof(char));
    *fanA = 'A';
    *fanB = 'B';
    for (int i = 0; i < totalFans; i++) 
    {
        if (i >= numberOfFansA && i >= numberOfFansB) {
            break;
        }
        if (i < numberOfFansA) {
            pthread_t threadA;
            pthread_create(&threadA, NULL, share_ride, (void*) fanA);
            fanThreads.push_back(threadA);
        }
        if (i < numberOfFansB) {
            pthread_t threadB;
            pthread_create(&threadB, NULL, share_ride, (void*) fanB);
            fanThreads.push_back(threadB);
        }
    }
    
    //wait for all fan threads to finish executing
    for (int i = 0; i < totalFans; i++) 
    {
        pthread_join(fanThreads[i], NULL);
    }
    //deallocate the memory and destory the last barrier created
    free(fanA); 
    free(fanB);
    pthread_barrier_destroy(&barrier); //last dummy barrier created by the last driver
    cout << "The main terminates" << endl;
    return 0;
}

void* share_ride(void* fanT) {
    char team = *((char*)fanT);
    bool isDriver = false;
    /*Reference variables to keep track of global values*/
    sem_t* waitingSemSameTeamFans, *waitingSemOtherTeamFans;
    int* waitingNumFansSameTeam, *waitingNumFansOtherTeam;

    //assign values accordingly
    if (team == 'A') {
        waitingSemSameTeamFans = &semWaitingFansA;
        waitingSemOtherTeamFans = &semWaitingFansB;

        waitingNumFansSameTeam = &waitingFansA;
        waitingNumFansOtherTeam = &waitingFansB;
    }
    else if (team == 'B') {
        waitingSemSameTeamFans = &semWaitingFansB;
        waitingSemOtherTeamFans = &semWaitingFansA;

        waitingNumFansSameTeam = &waitingFansB;
        waitingNumFansOtherTeam = &waitingFansA;
    }

    //lock the print mutex
    sem_wait(&printMutex);
    cout << "Thread ID: " << pthread_self() << ", Team: " << team << ", I am looking for a car" << endl;
    sem_post(&printMutex);
    /*
    To form a team, I need 3 people. Either 1 person from my team, 2 from the other or
    all 3 people from my own team. In either case, I am the driver as the latest person to arrive.
    */
   //3 people from my own team
   sem_wait(&checkMutex);
   if (*waitingNumFansSameTeam >= 3) {
        isDriver = true;
        *waitingNumFansSameTeam -= 3; //update the global value
        carID++;
        for (int i = 1; i <= 3; i++) {
            sem_post(waitingSemSameTeamFans); //signal 3 fans from my own team
        }
   }
   //1 from my team, 2 from the other
   else if (*waitingNumFansSameTeam >= 1 && *waitingNumFansOtherTeam >= 2) {
        isDriver = true;
        //update the global values
        *waitingNumFansSameTeam -= 1;
        *waitingNumFansOtherTeam -= 2;
        carID++;
        sem_post(waitingSemSameTeamFans); //signal 1 fan from my team
        for (int i = 1; i <= 2; i++) {//signal 2 fans from the other team (team b)
            sem_post(waitingSemOtherTeamFans); 
        }
   }
   //can not form a team yet, need to wait
   else {
        *waitingNumFansSameTeam += 1;
        sem_post(&checkMutex); //unlock the mutex here to ensure concurrency
        sem_wait(waitingSemSameTeamFans); //wait until a driver arrives
   }
   //found a spot in a car
    sem_wait(&printMutex);
    cout << "Thread ID: " << pthread_self() << ", Team: " << team << ", I have found a spot in a car" << endl;
    sem_post(&printMutex);
   //lets form the ride!
    pthread_barrier_wait(&barrier); //let every ride member come together
   if (isDriver) {
        sem_wait(&printMutex);
        cout << "Thread ID: " << pthread_self() << ", Team: " << team << ", I am the captain and driving the car with ID " << carID << endl;
        sem_post(&printMutex);
        //all set, destroy the barrier
        //and make a new one for the next team (if exists)
        pthread_barrier_destroy(&barrier);
        pthread_barrier_init(&barrier, NULL, 4);
        sem_post(&checkMutex); //unlock the mutex
   }
}