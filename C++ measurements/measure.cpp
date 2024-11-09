#include <chrono>
#include <iostream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <pthread.h>

const int NUM_TESTS = 100;
const std::vector<int> ARRAY_SIZES = {1000, 10000, 100000, 1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000, 10000000};
const int CONTEXT_SWITCH_ITERATIONS = 1000;

double calculateAverage(const std::vector<double> &times)
{
    return std::accumulate(times.begin(), times.end(), 0.0) / times.size();
}

double calculateStandardDeviation(const std::vector<double> &times, double mean)
{
    double variance = 0.0;
    for (const auto &time : times)
    {
        variance += (time - mean) * (time - mean);
    }
    return std::sqrt(variance / times.size());
}

void removeOutliers(std::vector<double> &times)
{
    double mean = calculateAverage(times);
    double stdDev = calculateStandardDeviation(times, mean);

    double lowerThreshold = mean - 3 * stdDev;
    double upperThreshold = mean + 3 * stdDev;

    times.erase(std::remove_if(times.begin(), times.end(),
                               [lowerThreshold, upperThreshold](double time)
                               {
                                   return time < lowerThreshold || time > upperThreshold;
                               }),
                times.end());
}

double measureStaticMemoryAccess(int size)
{
    int staticArray[size];
    for (int i = 0; i < size; ++i)
    {
        staticArray[i] = i;
    }

    auto start = std::chrono::high_resolution_clock::now();
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum = staticArray[i];
    }
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double, std::micro>(end - start).count();
}

double measureDynamicMemoryAccess(int size)
{
    int *dynamicArray = new int[size];
    if (dynamicArray == nullptr)
    {
        std::cerr << "Memory allocation failed.\n";
        return -1;
    }
    for (int i = 0; i < size; ++i)
    {
        dynamicArray[i] = i;
    }

    auto start = std::chrono::high_resolution_clock::now();
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum = dynamicArray[i];
    }
    auto end = std::chrono::high_resolution_clock::now();

    delete[] dynamicArray;
    return std::chrono::duration<double, std::micro>(end - start).count();
}

double measureMemoryAllocation(int size)
{
    auto start = std::chrono::high_resolution_clock::now();
    int *array = new int[size];
    auto end = std::chrono::high_resolution_clock::now();
    delete[] array;
    return std::chrono::duration<double, std::micro>(end - start).count();
}

double measureMemoryDeallocation(int size)
{
    int *array = new int[size];
    auto start = std::chrono::high_resolution_clock::now();
    delete[] array;
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

double measureThreadCreationTime()
{
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t([] {});
    auto end = std::chrono::high_resolution_clock::now();
    t.join();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

double measureContextSwitchTime()
{
    std::mutex mtx;
    std::condition_variable cv;
    bool turn = true;
    std::atomic<int> switches(0);

    auto threadFunc = [&](bool myTurn)
    {
        for (int i = 0; i < CONTEXT_SWITCH_ITERATIONS / 2; ++i)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]
                    { return turn == myTurn; });
            turn = !myTurn;
            ++switches;
            lock.unlock();
            cv.notify_one();
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1(threadFunc, true);
    std::thread t2(threadFunc, false);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    // printf("Switches: %d\n", switches.load());
    return std::chrono::duration<double, std::micro>(end - start).count() / CONTEXT_SWITCH_ITERATIONS;
}

double measureThreadMigrationTime()
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    auto threadFunc = []()
    {
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i)
        {
            sum += i;
        }
    };

    std::thread t(threadFunc);

    CPU_SET(0, &cpuset);
    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        perror("Error setting thread affinity");
    }

    auto start = std::chrono::high_resolution_clock::now();

    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        perror("Error setting thread affinity");
    }

    auto end = std::chrono::high_resolution_clock::now();
    t.join();

    return std::chrono::duration<double, std::micro>(end - start).count();
}

int main()
{
    std::cout << std::fixed << std::setprecision(6);

    for (int size : ARRAY_SIZES)
    {
        std::vector<double> staticAccessTimes, dynamicAccessTimes, allocTimes, deallocTimes;

        for (int i = 0; i < NUM_TESTS; ++i)
        {
            staticAccessTimes.push_back(measureStaticMemoryAccess(size));
            dynamicAccessTimes.push_back(measureDynamicMemoryAccess(size));
            allocTimes.push_back(measureMemoryAllocation(size));
            deallocTimes.push_back(measureMemoryDeallocation(size));
        }

        removeOutliers(staticAccessTimes);
        removeOutliers(dynamicAccessTimes);
        removeOutliers(allocTimes);
        removeOutliers(deallocTimes);

        if (staticAccessTimes.empty() || dynamicAccessTimes.empty() || allocTimes.empty() || deallocTimes.empty())
        {
            std::cout << "Not enough valid data for array size: " << size << "\n";
            continue;
        }

        double staticAverage = calculateAverage(staticAccessTimes);
        double staticStdDev = calculateStandardDeviation(staticAccessTimes, staticAverage);
        double dynamicAverage = calculateAverage(dynamicAccessTimes);
        double dynamicStdDev = calculateStandardDeviation(dynamicAccessTimes, dynamicAverage);
        double allocAverage = calculateAverage(allocTimes);
        double allocStdDev = calculateStandardDeviation(allocTimes, allocAverage);
        double deallocAverage = calculateAverage(deallocTimes);
        double deallocStdDev = calculateStandardDeviation(deallocTimes, deallocAverage);

        std::cout << "Array Size: " << size << "\n";
        std::cout << "Static Memory Access - Average Time: " << staticAverage << " microseconds, "
                  << "Standard Deviation: " << staticStdDev << " microseconds.\n";
        std::cout << "Dynamic Memory Access - Average Time: " << dynamicAverage << " microseconds, "
                  << "Standard Deviation: " << dynamicStdDev << " microseconds.\n";
        std::cout << "Memory Allocation - Average Time: " << allocAverage << " microseconds, "
                  << "Standard Deviation: " << allocStdDev << " microseconds.\n";
        std::cout << "Memory Deallocation - Average Time: " << deallocAverage << " microseconds, "
                  << "Standard Deviation: " << deallocStdDev << " microseconds.\n";
        std::cout << "------------------------------------------\n";
    }

    std::vector<double> threadCreationTimes, contextSwitchTimes, threadMigrationTimes;

    for (int i = 0; i < NUM_TESTS; ++i)
    {
        threadCreationTimes.push_back(measureThreadCreationTime());
        contextSwitchTimes.push_back(measureContextSwitchTime());
        threadMigrationTimes.push_back(measureThreadMigrationTime());
    }

    removeOutliers(threadCreationTimes);
    removeOutliers(contextSwitchTimes);
    removeOutliers(threadMigrationTimes);
    if (threadCreationTimes.empty() || contextSwitchTimes.empty() || threadMigrationTimes.empty())
    {
        std::cout << "Not enough valid data for thread creation, context switch, or thread migration.\n";
    }
    else
    {
        double threadCreationAverage = calculateAverage(threadCreationTimes);
        double threadCreationStdDev = calculateStandardDeviation(threadCreationTimes, threadCreationAverage);
        double contextSwitchAverage = calculateAverage(contextSwitchTimes);
        double contextSwitchStdDev = calculateStandardDeviation(contextSwitchTimes, contextSwitchAverage);
        double threadMigrationAverage = calculateAverage(threadMigrationTimes);
        double threadMigrationStdDev = calculateStandardDeviation(threadMigrationTimes, threadMigrationAverage);

        std::cout << "Thread Creation - Average Time: " << threadCreationAverage << " microseconds, "
                  << "Standard Deviation: " << threadCreationStdDev << " microseconds.\n";
        std::cout << "Context Switch - Average Time: " << contextSwitchAverage << " microseconds, "
                  << "Standard Deviation: " << contextSwitchStdDev << " microseconds.\n";
        std::cout << "Thread Migration - Average Time: " << threadMigrationAverage << " microseconds, "
                  << "Standard Deviation: " << threadMigrationStdDev << " microseconds.\n";
        std::cout << "------------------------------------------\n";
    }

    return 0;
}