#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>
#include "BenchmarkEngine.h"
JNIEXPORT jdouble JNICALL Java_BenchmarkEngine_measureThreadMigrationTime(JNIEnv *env, jobject obj, jint iterations) {
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
            //exit(EXIT_FAILURE);
        }
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < iterations; i++)
        {
            CPU_ZERO(&cpuset);
            CPU_SET(i % 2, &cpuset);
            int s2 = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
            if (s2 != 0)
            {
                perror("Failed to set affinity in iteration");
                //exit(EXIT_FAILURE);
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (pthread_join(thread, NULL) != 0)
        {
            perror("Failed to join thread");
            exit(EXIT_FAILURE);
        }
        double time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        return time / iterations;

}
