#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>

#define SHMKEY 123456
#define BUFF_SIZE sizeof(int)

int main(int argc, char* argv[]){
    int shmid = shmget(SHMKEY, BUFF_SIZE, 0777);
    if(shmid == -1){
        perror("ERROR in child  shared memory");
        exit(0);
    }
    int *cTimeGrab = (int*)(shmat(shmid, 0, 0));
    int iniTime = *cTimeGrab;
    int i;
    int flag = 0;
    int number = atoi(argv[1]);
    for(i = 2; i <= number / 2; i++){
        int * cCurrent = (int*)(shmat(shmid, 0, 0));
        if(*cCurrent - iniTime >= 100000){
            printf("failed to compute %d\n" , number);
            flag = -1;
            return(-1);
        }
        if(number % i == 0){
            flag = 1;

        }
    }

    if(flag == 0)
        return(0);
    else if(flag == 1)
        return(1);

}