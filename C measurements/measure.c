#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <stdint.h>

#define NUM_TESTS 100
#define CONTEXT_SWITCH_ITERATIONS 1000
#define NUM_ARRAY_SIZES 13

int ARRAY_SIZES[NUM_ARRAY_SIZES] = {1000, 10000, 100000, 1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000, 10000000};

double calculateAverage(double *times, int size)
{
    double sum = 0.0;
    for (int i = 0; i < size; i++)
    {
        sum += times[i];
    }
    return sum / size;
}

double calculateStandardDeviation(double *times, double mean, int size)
{
    double variance = 0.0;
    for (int i = 0; i < size; i++)
    {
        variance += (times[i] - mean) * (times[i] - mean);
    }
    return sqrt(variance / size);
}

void removeOutliers(double *times, int *size)
{
    double mean = calculateAverage(times, *size);
    double stdDev = calculateStandardDeviation(times, mean, *size);

    double lowerThreshold = mean - 3 * stdDev;
    double upperThreshold = mean + 3 * stdDev;

    int newSize = 0;
    for (int i = 0; i < *size; i++)
    {
        if (times[i] >= lowerThreshold && times[i] <= upperThreshold)
        {
            times[newSize++] = times[i];
        }
    }
    *size = newSize;
}

double measureStaticMemoryAccess(int size)
{
    int staticArray[size];
    for (int i = 0; i < size; i++)
    {
        staticArray[i] = i;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum = staticArray[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
}

double measureDynamicMemoryAccess(int size)
{
    int *dynamicArray = (int *)malloc(size * sizeof(int));
    if (dynamicArray == NULL)
    {
        perror("Failed to allocate memory for dynamicArray");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++)
    {
        dynamicArray[i] = i;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum = dynamicArray[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    free(dynamicArray);
    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
}

double measureMemoryAllocation(int size)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int *array = (int *)malloc(size * sizeof(int));
    if (array == NULL)
    {
        perror("Failed to allocate memory for allocation test");
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    free(array);
    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
}

double measureMemoryDeallocation(int size)
{
    int *array = (int *)malloc(size * sizeof(int));
    if (array == NULL)
    {
        perror("Failed to allocate memory for deallocation test");
        exit(EXIT_FAILURE);
    }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    free(array);
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
}

void *emptyThreadFunction(void *arg)
{
    return NULL;
}

double measureThreadCreationTime()
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_t thread;
    if (pthread_create(&thread, NULL, emptyThreadFunction, NULL) != 0)
    {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    if (pthread_join(thread, NULL) != 0)
    {
        perror("Failed to join thread");
        exit(EXIT_FAILURE);
    }
   
    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
}

double measureContextSwitchTime()
{
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int count = 0;
    int turn = 1;

    void *switchTask(void *arg)
    {
        int *turnPtr = (int *)arg;
        for (int i = 0; i < CONTEXT_SWITCH_ITERATIONS; ++i)
        {
            pthread_mutex_lock(&mutex);
            while (turn != *turnPtr)
            {
                pthread_cond_wait(&cond, &mutex);
            }
            turn = (turn == 1) ? 2 : 1;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        }
        return NULL;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t t1, t2;
    int arg1 = 1, arg2 = 2;
    if (pthread_create(&t1, NULL, switchTask, &arg1) != 0)
    {
        perror("Failed to create thread 1");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&t2, NULL, switchTask, &arg2) != 0)
    {
        perror("Failed to create thread 2");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(t1, NULL) != 0)
    {
        perror("Failed to join thread 1");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(t2, NULL) != 0)
    {
        perror("Failed to join thread 2");
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    return ((end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3) / CONTEXT_SWITCH_ITERATIONS;
}

double measureThreadMigrationTime()
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    void *migrationTask(void *arg)
    {
        volatile int sum = 0;
        for (int i = 0; i < 1000; i++)
        {
            sum += i;
        }
        return NULL;
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, migrationTask, NULL) != 0)
    {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    CPU_SET(0, &cpuset);
    int s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if(s != 0)
    {
        perror("Failed to set affinity");
        exit(EXIT_FAILURE);
    }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    int s2 = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if(s2 != 0)
    {
        perror("Failed to set affinity");
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (pthread_join(thread, NULL) != 0)
    {
        perror("Failed to join thread");
        exit(EXIT_FAILURE);
    }

    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
}

int main()
{

    for (int j = 0; j < NUM_ARRAY_SIZES; j++)
    {
        int size = ARRAY_SIZES[j];
        double staticAccessTimes[NUM_TESTS], dynamicAccessTimes[NUM_TESTS], allocTimes[NUM_TESTS], deallocTimes[NUM_TESTS];
        int staticSize = 0, dynamicSize = 0, allocSize = 0, deallocSize = 0;

        for (int i = 0; i < NUM_TESTS; i++)
        {
            staticAccessTimes[staticSize++] = measureStaticMemoryAccess(size);
            dynamicAccessTimes[dynamicSize++] = measureDynamicMemoryAccess(size);
            allocTimes[allocSize++] = measureMemoryAllocation(size);
            deallocTimes[deallocSize++] = measureMemoryDeallocation(size);
        }

        removeOutliers(staticAccessTimes, &staticSize);
        removeOutliers(dynamicAccessTimes, &dynamicSize);
        removeOutliers(allocTimes, &allocSize);
        removeOutliers(deallocTimes, &deallocSize);

        if (staticSize == 0 || dynamicSize == 0 || allocSize == 0 || deallocSize == 0)
        {
            printf("Not enough valid data for array size: %d\n", size);
            continue;
        }
        double staticAverage = calculateAverage(staticAccessTimes, staticSize);
        double staticStdDev = calculateStandardDeviation(staticAccessTimes, staticAverage, staticSize);
        double dynamicAverage = calculateAverage(dynamicAccessTimes, dynamicSize);
        double dynamicStdDev = calculateStandardDeviation(dynamicAccessTimes, dynamicAverage, dynamicSize);
        double allocAverage = calculateAverage(allocTimes, allocSize);
        double allocStdDev = calculateStandardDeviation(allocTimes, allocAverage, allocSize);
        double deallocAverage = calculateAverage(deallocTimes, deallocSize);
        double deallocStdDev = calculateStandardDeviation(deallocTimes, deallocAverage, deallocSize);
        printf("Array Size: %d\n", size);
        printf("Static Memory Access - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", staticAverage, staticStdDev);
        printf("Dynamic Memory Access - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", dynamicAverage, dynamicStdDev);
        printf("Memory Allocation - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", allocAverage, allocStdDev);
        printf("Memory Deallocation - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", deallocAverage, deallocStdDev);
        printf("----------------------------------------\n");
    }

    double threadCreationTimes[NUM_TESTS], contextSwitchTimes[NUM_TESTS], migrationTimes[NUM_TESTS];
    int creationSize = 0, switchSize = 0, migrationSize = 0;

    for (int i = 0; i < NUM_TESTS; i++)
    {
        threadCreationTimes[creationSize++] = measureThreadCreationTime();
        contextSwitchTimes[switchSize++] = measureContextSwitchTime();
        migrationTimes[migrationSize++] = measureThreadMigrationTime();
    }

    removeOutliers(threadCreationTimes, &creationSize);
    removeOutliers(contextSwitchTimes, &switchSize);
    removeOutliers(migrationTimes, &migrationSize);
    if (creationSize == 0 || switchSize == 0 || migrationSize == 0)
    {
        printf("Not enough valid data for thread creation, context switch, or thread migration\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        double threadCreationAverage = calculateAverage(threadCreationTimes, creationSize);
        double threadCreationStdDev = calculateStandardDeviation(threadCreationTimes, threadCreationAverage, creationSize);
        double contextSwitchAverage = calculateAverage(contextSwitchTimes, switchSize);
        double contextSwitchStdDev = calculateStandardDeviation(contextSwitchTimes, contextSwitchAverage, switchSize);
        double threadMigrationAverage = calculateAverage(migrationTimes, migrationSize);
        double threadMigrationStdDev = calculateStandardDeviation(migrationTimes, threadMigrationAverage, migrationSize);
        printf("Thread Creation - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", threadCreationAverage, threadCreationStdDev);
        printf("Context Switch - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", contextSwitchAverage, contextSwitchStdDev);
        printf("Thread Migration - Average Time: %.6f microseconds, Standard Deviation: %.6f\n", threadMigrationAverage, threadMigrationStdDev);
        printf("----------------------------------------\n");
    }

    return 0;
}
