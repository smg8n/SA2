#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>

#include "oss.h"

// user program will be passed 2 arguments the sequence number and the number to test for primt.
//
// will be given two command line arguments when they were execed off, which is what child they are and the number they should test for primality.
int main(int argc, char* argv[]){
 
    key_t shmKEYClock = ftok("./oss", 116);
    key_t shmKEYArray = ftok("./user", 116);

    int shmid = shmget(shmKEYClock, BUFF_SIZE, 0666);
    if(shmid == -1){
        perror("ERROR in child shared memory");
        exit(0);
    }
    int *cTimeGrab = (int*)(shmat(shmid, 0, 0));

    struct Clock initTime;
    initTime.seconds = cTimeGrab[0];
    initTime.nanoseconds = cTimeGrab[1];

   /*
    In addition to this shared clock, it should also allocate in shared memory an array equal
    in size to the maximum amount of child processes that it will launch and it should initialize all the locations in this
    array to 0.
   */
    int shmid_array = shmget(shmKEYArray, ARRAY_BUFF_SIZE, 0666);
    if(shmid_array == -1){
        perror("ERROR in child  shared memory array");
        exit(0);
    }
    int *prime_array = (int*)(shmat(shmid_array, 0, 0));

    int i;
    int flag = 0;
    int my_id = atoi(argv[1]);  // what child they are
    int number = atoi(argv[2]); // the number they should test for primality.
    // It should then start trying to determine primality. Periodically
    for(i = 2; i <= number / 2; i++){

        struct Clock curTime;
        curTime.seconds = cTimeGrab[0];
        curTime.nanoseconds = cTimeGrab[1];
        if ( ((curTime.seconds*1000000000+curTime.nanoseconds)-(initTime.seconds*1000000000+initTime.nanoseconds)) >= 1000000 ){
            // I do not want any child process to run for any more than 1 millisecond of simulated time. 1000000 ns == 1 millisec
            //printf("failed to compute %d in alloted time!!\n" , number);
            prime_array[my_id-1] = -1; // if more than a millisecond has passed in this time, it should finish by putting -1 in its associated location in the shared memory array.
            shmdt(cTimeGrab);
            shmdt(prime_array);
            flag = -1;
            return(2);
        }
        if(number % i == 0){
            flag = 1;
            break;
        }
    }

    if(flag == 0) {
        prime_array[my_id-1] = number; // If it is able to determine if the number is prime, it should place that number in the array.
        shmdt(cTimeGrab);
        shmdt(prime_array);
        return(0); // number is a prime number
    } else if(flag == 1) {
        prime_array[my_id-1] = -1*number; // If it determines that the number is not prime, it should put the negative of that number in the array.
        shmdt(cTimeGrab);
        shmdt(prime_array);
        return(1); // number is NOT a prime number
    }

}
