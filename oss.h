
#define SHMKEY_CLOCK 123456
#define BUFF_SIZE 2*sizeof(int)

#define MAX_CHILD_PROCESSES 1024
#define SHMKEY_ARRAY 1234567
#define ARRAY_BUFF_SIZE (MAX_CHILD_PROCESSES*sizeof(int))

struct Clock{
    int seconds;
    int nanoseconds;
    long totaltimeconsumed;
};

