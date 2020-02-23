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

/*
 Since checking primality can take a long time, we want to set a maximum amount of time for a child to try and
determine if a number is prime and we also want a way for a child to tell the master process that it has finished. We
will accomplish this synchronization with shared memory. oss will also be responsible for creating shared memory
to store two integers and then initializing those integers to zero. This shared memory should be accessible by the
children and will act as a clockin that one integer represents seconds, the other one represents nanoseconds. Note
that this is our simulated clock and will not have any correlation to real time. oss will be responsible for advancing
this clock by incrementing it at various times as described later. Note that child processes do not ever increment the
clock, they only look at it. In addition to this shared clock, it should also allocate in shared memory an array equal
in size to the maximum amount of child processes that it will launch and it should initialize all the locations in this
array to 0.
 */

#include "oss.h"

// incrementing the clock by 10000 (you can vary this number for testing in order to speed or slow down your clock).
#define CLOCK_INCREMENT 10000


static FILE *fn = NULL;

static int shmid = 0;
static int* shared_memory; // clock
static int shmid_array = 0;
static int* shared_memory_array;

static int maxchildren;
static int pnumber = 0;	// The start of numbers to be tested for primality
static int incrementnumber = 0;	// Increment between the numbers we test

static void outputSummary() 
{
    int* prime_array = shared_memory_array;
    // Lastly, it should display a list of the numbers
    // that it determined were prime and the numbers that it determined were not prime, followed by a list of numbers
    // that it did not have the time to make a determination.
    fprintf(fn, "The prime numbers are: \n");
    for (int i=0;i<maxchildren; i++) {
      if (prime_array[i] > 0)
        fprintf(fn,"%d ", prime_array[i]); 
    }
    fprintf(fn, "\nnon prime numbers are: \n");
    for (int i=0;i<maxchildren; i++) {
      if (prime_array[i] != -1 && prime_array[i] < 0)
        fprintf(fn,"%d ", -1*prime_array[i]); 
    }
    fprintf(fn, "\ndid not have the time to make a determination  numbers are: \n");
    for (int i=0;i<maxchildren; i++) {
      if (prime_array[i] == -1 || prime_array[i] ==0 )
        fprintf(fn,"%d ", pnumber+(i*incrementnumber));
    }
  // No matter how it terminated, oss should also output the value of the shared clock to the output file.
  fprintf(fn, "\nfinished at time %d seconds %d nanoseconds \n",  shared_memory[0],  shared_memory[1]);
  fclose(fn);
}

/* remove shared mem and exit */
void removeShmAndExit()
{
  /* Detach the shared memory segment.  */ 
  shmdt (shared_memory); 
  shmdt (shared_memory_array); 

  /* Deallocate the shared memory segment.  */ 
  shmctl (shmid, IPC_RMID, 0); 
  shmctl (shmid_array, IPC_RMID, 0); 
  kill(-1*getpid(), SIGKILL) ; // should kill oss process and any child user processes.
  exit(0); // this is unnecessary
}

/*Function handles ctrl c signal */
void ctrlCHandler(){
    char errorArr[200];
    snprintf(errorArr, 200, "\n\nCTRL+C signal caught, killing all processes and releasing shared memory.");
    perror(errorArr);
   outputSummary();
   removeShmAndExit();
}

/*Function handles timer alarm signal */
void timerHandler(){
    char errorArr[200];
    snprintf(errorArr, 200, "\n\ntimer interrupt triggered, killing all processes and releasing shared memory.");
    perror(errorArr);
   outputSummary();
    removeShmAndExit();
}

int main(int argc, char* argv[]){
    int options= 0;		// Placeholder for arguments
    maxchildren = 4;	// Max total number of child processes oss wil create.
    int noofchilds = 2;	// The number of children processes that can exists at any given time.
    pnumber = 0;	// The start of numbers to be tested for primality
    incrementnumber = 0;	// Increment between the numbers we test
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
                maxchildren = atoi(optarg);
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
    int primenumberarray[maxchildren];
    primenumberarray[0] = pnumber;
    for(i = 1; i < maxchildren; i++){
        primenumberarray[i] = primenumberarray[i - 1] + incrementnumber;
    }

    // arrays for primes
    int primes[maxchildren];
    int pcount = 0;
    //array for non prime numbers
    int noprimes[maxchildren];
    int npcount = 0;

    // attributes for the fork loop
    int activechild = 0;
    int childfinish = 0;
    int exitcount = 0;
    pid_t pid;
    int status;

    int id2pid[maxchildren]; // array to store id 2 pid mapping
    for(i = 1; i < maxchildren; i++){
        id2pid[i] = 0;
    }

    // file operations starts
    fn = fopen(outputfile, "w");
    if(!fn){
        perror("ERROR, could not open file\n");
        return(1);
    }

    // Clock Setup
    struct Clock timer;
    timer.seconds = 0;
    timer.nanoseconds = 0;

    // Shared Memory variables and error handeling
 
    key_t shmKEYClock = ftok("./oss", 116);
    key_t shmKEYArray = ftok("./user", 116);

    int shmid = shmget(shmKEYClock, BUFF_SIZE, 0666 | IPC_CREAT);
    if(shmid == -1){
        perror("ERROR in Parent for shmget\n");
        exit(1);
    }

    int* paddress = (int*)(shmat(shmid, 0, 0));
    int* ptime = (int*)(paddress);
    shared_memory = paddress; // share in static variable so it can be detached from

    // create shared memory array using child (user) will inform if a number is prime or not
    int shmid_array = shmget(shmKEYArray, ARRAY_BUFF_SIZE, IPC_CREAT|0666);
    if(shmid_array == -1){
        perror("ERROR in Parent shared memory array");
        exit(0);
    }
    int *prime_array = (int*)(shmat(shmid_array, 0, 0));
    shared_memory_array = prime_array; // save it so it can be detached later on
    // initialize to 0
    for (int i=0;i<maxchildren; i++) {
      prime_array[i] = 0;
    }
    //


    /*Catch ctrl+c signal and handle with ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm handle with timer Handler*/
    signal (SIGALRM, timerHandler);

    alarm( 2 ); // set timeout of 2 seconds.


    while(childfinish <= maxchildren && exitcount < maxchildren){
        int j=1;
        // incrementing the clock by 10000
        timer.nanoseconds += CLOCK_INCREMENT;
        if(timer.nanoseconds > 1000000000){
            timer.seconds++;
            timer.nanoseconds = 0;
        }
        ptime[0] = timer.seconds;
        ptime[1] = timer.nanoseconds;
        timer.totaltimeconsumed += CLOCK_INCREMENT;
        //
        // It should then see if any of the child processes
        // have terminated. If any of the child processes have terminated. If so, it should first check to see the result of that
        // child process (which it can look for in the shared memory array) and print the results to the log file.
        //
        if((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0){
            if(WIFEXITED(status)){
                int exitStatus = WEXITSTATUS(status);
                // map pid to id
                int id=-1;
                for (int i=0; i<childfinish; i++) {
                  if (id2pid[i] == pid) {
                    id = i;
                    break;
                  }
                }
                fprintf(fn, "Child with PID:%d , Child:%d has exited after %d seconds and %d nanoseconds.\n",pid, id, timer.seconds, timer.nanoseconds);
                if(exitStatus == 0){
                    primes[pcount] = prime_array[id];
                    pcount++;
                }else if(exitStatus == 1){
                    noprimes[npcount] = -1*prime_array[id];
                    npcount++;
                }
                activechild--;
                exitcount++;
            }
        }
        // It should then fork and exec off another child if we have not exceeded the maximum number of child processes to launch.
        if(childfinish < maxchildren && activechild < noofchilds){
            pid = fork();
            if(pid < 0){
                perror("fork failed\n");
                fclose(fn);
                exit(0);
            }
            else if(pid == 0){
                char convertid[15];
                char convertednumber[15];
                sprintf(convertid, "%d", childfinish);
                sprintf(convertednumber, "%d", primenumberarray[childfinish]);
                char *args[] = {"./user", convertid, convertednumber, NULL};
                execvp(args[0], args);
            }
            id2pid[childfinish] = pid;
            fprintf(fn, "Child with PID %d, Child %d and number %d has launched at time %d seconds and %d nanoseconds\n" ,pid, childfinish, primenumberarray[childfinish], timer.seconds, timer.nanoseconds);
            childfinish++;
            activechild++;
        }
    }
    outputSummary();
/*
    jmp_buf ctrlCjmp;
    jmp_buf timerjmp;
    //Jump back to main enviroment where children are launched but before the wait
    if(setjmp(ctrlCjmp) == 1){
        kill(pid, SIGKILL);
    }

    if(setjmp(timerjmp) == 1){
        kill(pid, SIGKILL);
    }
*/

  /* Detach the shared memory segment.  */ 
  shmdt (shared_memory); 
  shmdt (shared_memory_array); 

  /* Deallocate the shared memory segment.  */ 
  shmctl (shmid, IPC_RMID, 0); 
  shmctl (shmid_array, IPC_RMID, 0); 

  return 0;
}
