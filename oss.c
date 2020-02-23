
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#define SHMKEY 	123456
#define BUFF_SIZE sizeof(int)

struct Clock{
    int seconds;
    int nanoseconds;
    long totaltimecomsumed;
};
/*Function handles ctrl c signal */
void ctrlCHandler(){
    char errorArr[200];
    snprintf(errorArr, 200, "\n\nCTRL+C signal caught, killing all processes and releasing shared memory.");
    perror(errorArr);
    exit(0);

}

/*Function handles timer alarm signal */
void timerHandler(){
    char errorArr[200];
    snprintf(errorArr, 200, "\n\ntimer interrupt triggered, killing all processes and releasing shared memory.");
    perror(errorArr);
    exit(0);


}
int main(int argc, char* argv[]){
    int options= 0;		// Placeholder for arguments
    int maxchilderns = 4;	// Max total number of child processes oss wil create.
    int noofchilds = 2;	// The number of children processes that can exists at any given time.
    int pnumber = 0;	// The start of numbers to be tested for primality
    int incrementnumber = 0;	// Increment between the numbers we test
    char outputfile[255] = "output.log";	// Output file


    while((options = getopt(argc, argv, "hn:s:b:i:o:")) != -1)
        switch(options){
            case 'h':
                printf("This program will detect if numbers within a set are prime using child processes\n");
                printf("-h Will display a help message of what each command line object does and then will exist the program\n");
                printf("-s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n");
                printf("-b B Start of the sequence of numbers to be tested for primality\n");
                printf("-i I Increment between numbers that we test\n");
                printf("-o filename Output file\n");
                return EXIT_SUCCESS;
            case 'n':
                maxchilderns = atoi(optarg);
                break;
            case 's':
                noofchilds = atoi(optarg);
                break;
            case 'b':
                pnumber = atoi(optarg);
                break;
            case 'i':
                incrementnumber = atoi(optarg);
                break;
            case 'o':
                strncpy(outputfile, optarg, 255);
                break;
            default:
                return EXIT_FAILURE;

    }
    // array to store increments for prime numbers
    int i;
    int primenumberarray[maxchilderns];
    primenumberarray[0] = pnumber;
    for(i = 1; i < maxchilderns; i++){
        primenumberarray[i] = primenumberarray[i - 1] + incrementnumber;
    }

    // arrays for primes
    int primes[maxchilderns];
    int pcount = 0;
    //array for non prime numbers
    int noprimes[maxchilderns];
    int npcount = 0;

    // attributes for the fork loop
    int activechild = 0;
    int childfinish = 0;
    int exitcount = 0;
    pid_t pid;
    int status;

    // file operations starts
    FILE *fn = fopen(outputfile, "w");
    if(!fn){
        perror("ERROR, could not open file\n");
        return(1);
    }

    // Clock Setup
    struct Clock timer;
    timer.seconds = 0;
    timer.nanoseconds = 0;

    // Shared Memory variables and error handeling
    int shmid = shmget(SHMKEY, BUFF_SIZE, 0777 | IPC_CREAT);
    if(shmid == -1){
        perror("ERROR in Parent for shmget\n");
        exit(1);
    }

    char* paddress = (char*)(shmat(shmid, 0, 0));
    int* ptime = (int*)(paddress);



    while(childfinish <= maxchilderns && exitcount < maxchilderns){
        int j=1;
        *ptime = timer.totaltimecomsumed;
        timer.nanoseconds += 1;
        timer.totaltimecomsumed += 1;
        if(timer.nanoseconds > 1000000000){
            timer.seconds++;
            timer.nanoseconds = 1000000000 - timer.nanoseconds;
        }
        if(childfinish < maxchilderns && activechild < noofchilds){
            pid = fork();
            if(pid < 0){
                perror("fork failed\n");
                fclose(fn);
                exit(0);
            }
            else if(pid == 0){
                char convertednumber[15];
                char convertpid[15];
                sprintf(convertednumber, "%d", primenumberarray[childfinish]);
                sprintf(convertpid, "%d", getpid());
                char *args[] = {"./user", convertednumber, convertpid, NULL};
                execvp(args[0], args);


            }
            fprintf(fn, "Child with PID %d and number %d has launched at time %d seconds and %d nanoseconds\n" ,pid, primenumberarray[childfinish], timer.seconds, timer.nanoseconds);
            childfinish++;
            activechild++;
        }
        if((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0){
            if(WIFEXITED(status)){
                int exitStatus = WEXITSTATUS(status);
                fprintf(fn, "Child with PID:%d has been terminated after %d seconds and %d nanoseconds\n",pid, timer.seconds, timer.nanoseconds);
                if(exitStatus == 0){
                    primes[pcount] = primenumberarray[exitcount];
                    pcount++;
                }else if(exitStatus == 1){
                    noprimes[npcount] = primenumberarray[exitcount];
                    npcount++;
                }
                activechild--;
                exitcount++;
            }
        }
    }
    fprintf(fn, "The prime numbers are: \n");
    for(i = 0; i < pcount; i++){
        fprintf(fn, "%d ", primes[i]);
    }
    fprintf(fn, "\nnon prime numbers are: \n");
    for(i = 0; i < npcount; i++){
        fprintf(fn, "%d ", -noprimes[i]);
    }
    fprintf(fn, "\nfinished at time %f seconds \n",  timer.nanoseconds/1000000000.0);

    fclose(fn);
    /*Catch ctrl+c signal and handle with
    *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm handle with timer Handler*/
    signal (SIGALRM, timerHandler);

    alarm( 2 ); // set timeout of 2 seconds.

    jmp_buf ctrlCjmp;
    jmp_buf timerjmp;
    /*Jump back to main enviroment where children are launched
*      * but before the wait*/
/*
    if(setjmp(ctrlCjmp) == 1){
        kill(pid, SIGKILL);
    }

    if(setjmp(timerjmp) == 1){
        kill(pid, SIGKILL);
    }
*/

    return 0;
}
