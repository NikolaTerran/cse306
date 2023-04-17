#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define BUFFER_SIZE 1024
#define NUM_ITERATIONS 10000

int main() {
    //it's all zeros
    char buffer[BUFFER_SIZE];

    // Write benchmark
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int fp = open("test.txt", 0);
        write(fp, buffer, BUFFER_SIZE);
        close(fp);
        // printf(1,"%d/%d\n",i+1,NUM_ITERATIONS);
    }
    printf(1,"Write complete\n");

    // Read benchmark
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int fp = open("test.txt", 0);
        read(fp, buffer, BUFFER_SIZE);
        close(fp);
        // printf(1,"%d/%d\n",i+1,NUM_ITERATIONS);
    }
    printf(1,"Read complete\n");
    exit();
}