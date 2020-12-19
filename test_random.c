#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{

    struct timespec ttime = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ttime);
    unsigned int seed = ttime.tv_sec+ttime.tv_nsec;
    srandom(seed);
    for (int i = 0; i < 10; i++) {
        printf("random:%d\n",10*random()/RAND_MAX);
    }
    
    return 0;
}
