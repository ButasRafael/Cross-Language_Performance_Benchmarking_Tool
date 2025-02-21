#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include <stdatomic.h>

#define NUM_TESTS 100
#define CREATION_ITERATIONS 10000
#define CONTEXT_SWITCH_ITERATIONS 10000
#define MIGRATION_ITERATIONS 10000
#define NUM_ARRAY_SIZES 8

int ARRAY_SIZES[NUM_ARRAY_SIZES] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};

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

void removeOutliers(double *times, int *size, double threshold)
{
    double mean = calculateAverage(times, *size);
    double stdDev = calculateStandardDeviation(times, mean, *size);

    double lowerThreshold = mean - threshold * stdDev;
    double upperThreshold = mean + threshold * stdDev;

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
        sum += staticArray[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / size;
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
        sum += dynamicArray[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    free(dynamicArray);
    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / size;
}

double measureMemoryAllocation(int size)
{
    struct timespec start, end;
    int **chunks = (int **)malloc(size * sizeof(int *));
    if (chunks == NULL)
    {
        perror("Failed to allocate memory for chunk pointers");
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < size; i++)
    {
        chunks[i] = (int *)malloc(sizeof(int));
        if (chunks[i] == NULL)
        {
            perror("Failed to allocate memory for a chunk");
            exit(EXIT_FAILURE);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    for (int i = 0; i < size; i++)
    {
        free(chunks[i]);
    }
    free(chunks);

    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / size;
}

double measureMemoryDeallocation(int size)
{
    struct timespec start, end;
    void **chunks = malloc(size * sizeof(void *));
    if (!chunks)
    {
        perror("Failed to allocate memory for chunk pointers");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++)
    {
        chunks[i] = (int *)malloc(sizeof(int));
        if (!chunks[i])
        {
            perror("Failed to allocate memory for a chunk");
            exit(EXIT_FAILURE);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < size; i++)
    {
        free(chunks[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    free(chunks);

    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / size;
}

void *CreateThreadFunction(void *arg)
{
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++)
    {
        sum += i;
    }
    return NULL;
}

double measureThreadCreationTime()
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < CREATION_ITERATIONS; i++)
    {
        pthread_t thread;
        if (pthread_create(&thread, NULL, CreateThreadFunction, NULL) != 0)
        {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
        if (pthread_join(thread, NULL) != 0)
        {
            perror("Failed to join thread");
            exit(EXIT_FAILURE);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / CREATION_ITERATIONS;
}

double measureContextSwitchTime()
{
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int turn = 1;

    void *switchTask(void *arg)
    {
        int *turnPtr = (int *)arg;
        for (int i = 0; i < CONTEXT_SWITCH_ITERATIONS / 2; ++i)
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
    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / CONTEXT_SWITCH_ITERATIONS;
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
    if (s != 0)
    {
        perror("Failed to set affinity");
        exit(EXIT_FAILURE);
    }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < MIGRATION_ITERATIONS; i++)
    {
        CPU_ZERO(&cpuset);
        CPU_SET(i % 2, &cpuset);
        int s2 = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
        if (s2 != 0)
        {
            perror("Failed to set affinity in iteration");
            exit(EXIT_FAILURE);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (pthread_join(thread, NULL) != 0)
    {
        perror("Failed to join thread");
        exit(EXIT_FAILURE);
    }
    double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return time / MIGRATION_ITERATIONS;
}

void saveResultsToJSON(FILE *file, double average, double stdDev, const char *process, int numTests, int passedTests, const char *language, int arraySize, double threshold, int isFirstEntry)
{
    if (!isFirstEntry)
    {
        fprintf(file, ",\n");
    }

    cJSON *json = cJSON_CreateObject();
    if (arraySize > 0)
    {
        cJSON_AddNumberToObject(json, "array_size", arraySize);
    }
    cJSON_AddNumberToObject(json, "number_of_tests", numTests);
    cJSON_AddNumberToObject(json, "passed_tests", passedTests);
    cJSON_AddNumberToObject(json, "outlier_threshold", threshold);
    cJSON_AddStringToObject(json, "programming_language", language);
    cJSON_AddStringToObject(json, "process_measured", process);
    cJSON_AddNumberToObject(json, "average_time", average);
    cJSON_AddNumberToObject(json, "std_deviation", stdDev);

    char *jsonString = cJSON_Print(json);
    if (jsonString == NULL)
    {
        perror("Failed to print JSON string");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", jsonString);
    fflush(file);
    free(jsonString);
    cJSON_Delete(json);
}

void combineJSONFiles(const char *outputFilename, const char **filenames, int numFiles)
{
    cJSON *combinedResults = cJSON_CreateArray();

    for (int i = 0; i < numFiles; i++)
    {
        FILE *inputFile = fopen(filenames[i], "r");
        if (inputFile == NULL)
        {
            perror("Failed to open input file");
            continue;
        }

        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        fseek(inputFile, 0, SEEK_SET);

        char *fileContent = (char *)malloc(fileSize + 1);
        if (fileContent == NULL)
        {
            perror("Failed to allocate memory for file content");
            fclose(inputFile);
            continue;
        }

        fread(fileContent, 1, fileSize, inputFile);
        fileContent[fileSize] = '\0';
        fclose(inputFile);

        cJSON *fileJSON = cJSON_Parse(fileContent);
        free(fileContent);

        if (fileJSON == NULL)
        {
            const char *errorPtr = cJSON_GetErrorPtr();
            if (errorPtr != NULL)
            {
                fprintf(stderr, "Failed to parse JSON content in file %s: %s\n", filenames[i], errorPtr);
            }
            else
            {
                fprintf(stderr, "Failed to parse JSON content in file %s\n", filenames[i]);
            }
            continue;
        }

        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, fileJSON)
        {
            cJSON_AddItemToArray(combinedResults, cJSON_Duplicate(entry, 1));
        }

        cJSON_Delete(fileJSON);
    }

    char *jsonString = cJSON_Print(combinedResults);
    if (jsonString == NULL)
    {
        perror("Failed to print JSON string");
        exit(EXIT_FAILURE);
    }

    FILE *outputFile = fopen(outputFilename, "w");
    if (outputFile == NULL)
    {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }
    fprintf(outputFile, "%s\n", jsonString);
    fflush(outputFile);
    fclose(outputFile);
    free(jsonString);
    cJSON_Delete(combinedResults);
}

void StaticAccessMain(int numTests, double threshold)
{
    double staticAccessTimes[numTests];
    FILE *staticAccessFile = fopen("C_static_access.json", "w");
    if (staticAccessFile == NULL)
    {
        perror("Failed to open static_access.json");
        exit(EXIT_FAILURE);
    }
    fprintf(staticAccessFile, "[\n");

    for (int j = 0; j < NUM_ARRAY_SIZES; j++)
    {
        int size = ARRAY_SIZES[j];
        int staticSize = 0;

        for (int i = 0; i < numTests; i++)
        {
            staticAccessTimes[staticSize++] = measureStaticMemoryAccess(size);
        }

        removeOutliers(staticAccessTimes, &staticSize, threshold);

        if (staticSize)
        {
            double staticAverage = calculateAverage(staticAccessTimes, staticSize);
            double staticStdDev = calculateStandardDeviation(staticAccessTimes, staticAverage, staticSize);
            saveResultsToJSON(staticAccessFile, staticAverage, staticStdDev, "Static Memory Access", numTests, staticSize, "C", size, threshold, (j == 0));
        }
        else
        {
            fprintf(stderr, "All static memory access times were outliers for array size %d\n", size);
        }
    }

    fprintf(staticAccessFile, "\n]");
    fclose(staticAccessFile);
}

void DynamicAccessMain(int numTests, double threshold)
{
    double dynamicAccessTimes[numTests];
    FILE *dynamicAccessFile = fopen("C_dynamic_access.json", "w");
    if (dynamicAccessFile == NULL)
    {
        perror("Failed to open dynamic_access.json");
        exit(EXIT_FAILURE);
    }
    fprintf(dynamicAccessFile, "[\n");

    for (int j = 0; j < NUM_ARRAY_SIZES; j++)
    {
        int size = ARRAY_SIZES[j];
        int dynamicSize = 0;

        for (int i = 0; i < numTests; i++)
        {
            dynamicAccessTimes[dynamicSize++] = measureDynamicMemoryAccess(size);
        }

        removeOutliers(dynamicAccessTimes, &dynamicSize, threshold);

        if (dynamicSize)
        {
            double dynamicAverage = calculateAverage(dynamicAccessTimes, dynamicSize);
            double dynamicStdDev = calculateStandardDeviation(dynamicAccessTimes, dynamicAverage, dynamicSize);
            saveResultsToJSON(dynamicAccessFile, dynamicAverage, dynamicStdDev, "Dynamic Memory Access", numTests, dynamicSize, "C", size, threshold, (j == 0));
        }
        else
        {
            fprintf(stderr, "All dynamic memory access times were outliers for array size %d\n", size);
        }
    }

    fprintf(dynamicAccessFile, "\n]");
    fclose(dynamicAccessFile);
}

void AllocationMain(int numTests, double threshold)
{
    double allocTimes[numTests];
    FILE *allocationFile = fopen("C_allocation.json", "w");
    if (allocationFile == NULL)
    {
        perror("Failed to open allocation.json");
        exit(EXIT_FAILURE);
    }
    fprintf(allocationFile, "[\n");

    for (int j = 0; j < NUM_ARRAY_SIZES; j++)
    {
        int size = ARRAY_SIZES[j];
        int allocSize = 0;

        for (int i = 0; i < numTests; i++)
        {
            allocTimes[allocSize++] = measureMemoryAllocation(size);
        }

        removeOutliers(allocTimes, &allocSize, threshold);

        if (allocSize)
        {
            double allocAverage = calculateAverage(allocTimes, allocSize);
            double allocStdDev = calculateStandardDeviation(allocTimes, allocAverage, allocSize);
            saveResultsToJSON(allocationFile, allocAverage, allocStdDev, "Memory Allocation", numTests, allocSize, "C", size, threshold, (j == 0));
        }
        else
        {
            fprintf(stderr, "All memory allocation times were outliers for array size %d\n", size);
        }
    }

    fprintf(allocationFile, "\n]");
    fclose(allocationFile);
}

void DeallocationMain(int numTests, double threshold)
{
    double deallocTimes[numTests];
    FILE *deallocationFile = fopen("C_deallocation.json", "w");
    if (deallocationFile == NULL)
    {
        perror("Failed to open deallocation.json");
        exit(EXIT_FAILURE);
    }
    fprintf(deallocationFile, "[\n");

    for (int j = 0; j < NUM_ARRAY_SIZES; j++)
    {
        int size = ARRAY_SIZES[j];
        int deallocSize = 0;

        for (int i = 0; i < numTests; i++)
        {
            deallocTimes[deallocSize++] = measureMemoryDeallocation(size);
        }

        removeOutliers(deallocTimes, &deallocSize, threshold);

        if (deallocSize)
        {
            double deallocAverage = calculateAverage(deallocTimes, deallocSize);
            double deallocStdDev = calculateStandardDeviation(deallocTimes, deallocAverage, deallocSize);
            saveResultsToJSON(deallocationFile, deallocAverage, deallocStdDev, "Memory Deallocation", numTests, deallocSize, "C", size, threshold, (j == 0));
        }
        else
        {
            fprintf(stderr, "All memory deallocation times were outliers for array size %d\n", size);
        }
    }

    fprintf(deallocationFile, "\n]");
    fclose(deallocationFile);
}

void ThreadCreationMain(int numTests, double threshold)
{
    double threadCreationTimes[numTests];
    FILE *threadCreationFile = fopen("C_thread_creation.json", "w");
    if (threadCreationFile == NULL)
    {
        perror("Failed to open thread_creation.json");
        exit(EXIT_FAILURE);
    }
    fprintf(threadCreationFile, "[\n");

    for (int i = 0; i < numTests; i++)
    {
        threadCreationTimes[i] = measureThreadCreationTime();
    }

    removeOutliers(threadCreationTimes, &numTests, threshold);

    if (numTests)
    {
        double creationAverage = calculateAverage(threadCreationTimes, numTests);
        double creationStdDev = calculateStandardDeviation(threadCreationTimes, creationAverage, numTests);
        saveResultsToJSON(threadCreationFile, creationAverage, creationStdDev, "Thread Creation", numTests, numTests, "C", 0, threshold, 1);
    }
    else
    {
        fprintf(stderr, "All thread creation times were outliers\n");
    }

    fprintf(threadCreationFile, "\n]");
    fclose(threadCreationFile);
}

void ContextSwitchMain(int numTests, double threshold)
{
    double contextSwitchTimes[numTests];
    FILE *contextSwitchFile = fopen("C_context_switch.json", "w");
    if (contextSwitchFile == NULL)
    {
        perror("Failed to open context_switch.json");
        exit(EXIT_FAILURE);
    }
    fprintf(contextSwitchFile, "[\n");

    for (int i = 0; i < numTests; i++)
    {
        contextSwitchTimes[i] = measureContextSwitchTime();
    }

    removeOutliers(contextSwitchTimes, &numTests, threshold);

    if (numTests)
    {
        double switchAverage = calculateAverage(contextSwitchTimes, numTests);
        double switchStdDev = calculateStandardDeviation(contextSwitchTimes, switchAverage, numTests);
        saveResultsToJSON(contextSwitchFile, switchAverage, switchStdDev, "Context Switch", numTests, numTests, "C", 0, threshold, 1);
    }
    else
    {
        fprintf(stderr, "All context switch times were outliers\n");
    }

    fprintf(contextSwitchFile, "\n]");
    fclose(contextSwitchFile);
}

void ThreadMigrationMain(int numTests, double threshold)
{
    double migrationTimes[numTests];
    FILE *migrationFile = fopen("C_thread_migration.json", "w");
    if (migrationFile == NULL)
    {
        perror("Failed to open thread_migration.json");
        exit(EXIT_FAILURE);
    }
    fprintf(migrationFile, "[\n");

    for (int i = 0; i < numTests; i++)
    {
        migrationTimes[i] = measureThreadMigrationTime();
    }

    removeOutliers(migrationTimes, &numTests, threshold);

    if (numTests)
    {
        double migrationAverage = calculateAverage(migrationTimes, numTests);
        double migrationStdDev = calculateStandardDeviation(migrationTimes, migrationAverage, numTests);
        saveResultsToJSON(migrationFile, migrationAverage, migrationStdDev, "Thread Migration", numTests, numTests, "C", 0, threshold, 1);
    }
    else
    {
        fprintf(stderr, "All thread migration times were outliers\n");
    }

    fprintf(migrationFile, "\n]");
    fclose(migrationFile);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <number_of_tests> <outlier_threshold>\n", argv[0]);
        return 1;
    }

    int numTests = atoi(argv[1]);
    double threshold = atof(argv[2]);

    StaticAccessMain(numTests, threshold);
    DynamicAccessMain(numTests, threshold);
    AllocationMain(numTests, threshold);
    DeallocationMain(numTests, threshold);
    ThreadCreationMain(numTests, threshold);
    ContextSwitchMain(numTests, threshold);
    ThreadMigrationMain(numTests, threshold);

    const char *filenames[] = {
        "C_static_access.json",
        "C_dynamic_access.json",
        "C_allocation.json",
        "C_deallocation.json",
        "C_thread_creation.json",
        "C_context_switch.json",
        "C_thread_migration.json"};
    int numFiles = sizeof(filenames) / sizeof(filenames[0]);

    combineJSONFiles("C_results.json", filenames, numFiles);

    return 0;
}

