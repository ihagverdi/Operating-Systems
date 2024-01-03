#include <iostream>
#include <cstdlib>
#include <pthread.h>
using namespace std;

/*
Author: Hagverdi Ibrahimli
Date: January 3rd, 2023
This program implements a heap manager simulator that supports malloc and free operations.
*/
class HeapManager {
    public:
        /*constructor*/
        HeapManager() {
            //initialize the lock
            lock = PTHREAD_MUTEX_INITIALIZER;
            heap = NULL;
        }
        /*returns 1 upon successful initialization of the heap*/
        int initHeap(int size) 
        {
            heap = new HeapNode(-1, size, 0);
            cout << "Memory initialized" << endl;
            pthread_mutex_lock(&lock);
            print();
            pthread_mutex_unlock(&lock);
            return 1;
        }
        /*returns the beginning address of the newly allocated space upon success, else returns -1*/
        int myMalloc(int id, int size) 
        {
            pthread_mutex_lock(&lock);
            //check if heap is initialized
            if (heap == NULL) {
                pthread_mutex_unlock(&lock);
                return -1;
            }
            HeapNode* curr = heap;
            int pos = 0; //position of insertion in the list
            while (curr != NULL) {
                    if (curr ->id == -1) {
                        if (curr->size > size) {
                            //split the chunk
                            HeapNode* newNode = new HeapNode(id, size, curr->index);
                            newNode->next = curr;
                            curr->index = curr->index + size;
                            curr->size = curr->size - size;

                            if (pos == 0) { //insert at the beginning
                                heap = newNode;
                            }
                            else {
                                HeapNode* prev = heap;
                                for (int i = 0; i < pos - 1; i++) {
                                    prev = prev->next;
                                }
                                prev->next = newNode;
                            }
                            cout << "Allocated for thread " << id << endl;
                            print();
                            pthread_mutex_unlock(&lock);
                            return newNode->index;
                        }
                        else if (curr->size == size) {
                            curr->id = id;
                            cout << "Allocated for thread " << id << endl;
                            print();
                            pthread_mutex_unlock(&lock);
                            return curr->index;
                        }
                    }
                    pos++; //increment position
                    curr = curr->next;
            }
            cout << "Can not allocate, requested size " << size << " for thread " << id << " is bigger than remaining size" << endl;
            print();
            pthread_mutex_unlock(&lock);
            return -1;
    }
        /*returns 1 on success, -1 on failure*/  
        int myFree(int id, int index) {
            pthread_mutex_lock(&lock);

            if (heap == NULL) {
                pthread_mutex_unlock(&lock);
                return -1;
            }
            int pos = 0; //position of the chunk to be deleted in the list
            HeapNode* curr = heap;
            while (curr != NULL) {
                if (curr->id == id && curr->index == index) {
                    HeapNode* left = heap;
                    for (int i = 0; i < pos - 1; i++) {
                        left = left->next;
                    }
                    //check if the left chunk is free
                    if (left != NULL && left->id == -1) {
                        left->size = left->size + curr->size;
                        left->next = curr->next;
                        delete curr;
                        curr = left;
                    }
                    curr->id = -1;
                    //check if the right chunk is free
                    HeapNode* right = curr->next;
                    if (right != NULL && right->id == -1) {
                        curr->size = curr->size + right->size;
                        curr->next = right->next;
                        delete right;
                    }
                    cout << "Freed for thread " << id << endl;
                    print();
                    pthread_mutex_unlock(&lock);
                    return 1;
                }
                pos++;
                curr = curr->next;
            }
            cout << "Can not free, no chunk allocated for thread " << id << " at address " << index << endl;
            print();
            pthread_mutex_unlock(&lock);
            return -1;
        }
        /*prints the memory layout*/
        void print() {
            HeapNode* curr = heap;
            while (curr != NULL) {
                cout << "[" << curr->id << "][" << curr->size << "][" << curr->index << "]";
                if (curr->next != NULL) {
                    cout << "---";
                }                
                curr = curr->next;
            }
            cout << endl;
        }
    private:
        // HeapNode struct
        struct HeapNode {
            /*node: {[id][size][index]}*/
                int id; //thread id of the thread that allocated this chunk
                int size; //size of the chunk
                int index; //starting address of the chunk
                HeapNode* next;
                HeapNode(int ID, int S, int IND) : id(ID), size(S), index(IND), next(NULL) {}
            };
        // head of the heap
        HeapNode* heap;
        //lock for the heap
        pthread_mutex_t lock;
};