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
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "JNInterface.h"
using ordered_json = nlohmann::ordered_json;

const int NUM_TESTS = 100;
const std::vector<int> ARRAY_SIZES = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
const std::vector<int> ITERATIONS = {2, 10, 100, 1000, 10000};

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

void removeOutliers(std::vector<double> &times, double threshold)
{
    double mean = calculateAverage(times);
    double stdDev = calculateStandardDeviation(times, mean);

    double lowerThreshold = mean - threshold * stdDev;
    double upperThreshold = mean + threshold * stdDev;

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
        sum += staticArray[i];
    }
    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / size;
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
        sum += dynamicArray[i];
    }
    auto end = std::chrono::high_resolution_clock::now();

    delete[] dynamicArray;
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / size;
}

double measureMemoryAllocation(int size)
{
    std::vector<int *> chunks(size);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < size; ++i)
    {
        chunks[i] = new int;
        if (chunks[i] == nullptr)
        {
            std::cerr << "Failed to allocate memory for a chunk" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < size; ++i)
    {
        delete chunks[i];
    }
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / size;
}

double measureMemoryDeallocation(int size)
{
    std::vector<int *> chunks(size);

    for (int i = 0; i < size; ++i)
    {
        chunks[i] = new int;
        if (chunks[i] == nullptr)
        {
            std::cerr << "Failed to allocate memory for a chunk" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < size; ++i)
    {
        delete chunks[i];
    }
    auto end = std::chrono::high_resolution_clock::now();

    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / size;
}

void CreateThreadFunction()
{
    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i)
    {
        sum += i;
    }
}

double measureThreadCreationTime(int iterations)
{
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        std::thread t(CreateThreadFunction);
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();

    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / iterations;
}

double measureContextSwitchTime(int iterations)
{
    std::mutex mtx;
    std::condition_variable cv;
    bool turn = true;

    auto threadFunc = [&](bool myTurn)
    {
        for (int i = 0; i < iterations / 2; ++i)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]
                    { return turn == myTurn; });
            turn = !myTurn;
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
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / iterations;
}

double measureThreadMigrationTime(int iterations)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    auto migrationTask = []()
    {
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i)
        {
            sum += i;
        }
    };

    std::thread t(migrationTask);

    CPU_SET(0, &cpuset);
    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        perror("Error setting thread affinity");
        //exit(EXIT_FAILURE);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        CPU_ZERO(&cpuset);
        CPU_SET(i % 2, &cpuset);
        rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0)
        {
            perror("Error setting thread affinity in iteration");
            //exit(EXIT_FAILURE);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    t.join();

    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time / iterations;
}

void ensureDirectoryExists(const std::string &folderName)
{
    if (!std::filesystem::exists(folderName))
    {
        std::filesystem::create_directory(folderName);
    }
}

void saveResultsToJSON(const std::string &filename, double average, double stdDev, const std::string &process, int numTests, int passedTests, const std::string &language, int arraySize, double threshold, int iterations)
{
    const std::string folderName = "C++_measurements";
    ensureDirectoryExists(folderName);

    std::string fullPath = folderName + "/" + filename;

    ordered_json json;
    std::ifstream file_in(fullPath);
    if (file_in.is_open())
    {
        file_in >> json;
        file_in.close();
    }
    else
    {
        json = ordered_json::array();
    }

    ordered_json result;
    if (arraySize > 0)
        result["array_size"] = arraySize;
    if (iterations > 0)
        result["iterations"] = iterations;
    result["number_of_tests"] = numTests;
    result["passed_tests"] = passedTests;
    result["outlier_threshold"] = threshold;
    result["programming_language"] = language;
    result["process_measured"] = process;
    result["average_time"] = average;
    result["std_deviation"] = stdDev;

    json.push_back(result);

    std::ofstream file_out(fullPath);
    file_out << json.dump(4);
    file_out.flush();
    file_out.close();
}

void combineJSONFiles(const std::vector<std::string> &filenames, const std::string &outputFilename)
{
    const std::string folderName = "C++_measurements";
    ensureDirectoryExists(folderName);

    ordered_json combinedResults = ordered_json::array();

    for (const auto &filename : filenames)
    {
        std::ifstream inputFile(folderName + "/" + filename);
        if (inputFile.is_open())
        {
            ordered_json fileContent;
            inputFile >> fileContent;
            inputFile.close();

            for (const auto &entry : fileContent)
            {
                combinedResults.push_back(entry);
            }
        }
        else
        {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }

    std::string fullPath = folderName + "/" + outputFilename;

    std::ofstream outputFile(fullPath);
    if (outputFile.is_open())
    {
        outputFile << combinedResults.dump(4);
        outputFile.close();
    }
    else
    {
        std::cerr << "Failed to open output file: " << outputFilename << std::endl;
    }
}

void StaticAccessMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_static_access.json") << "[]";

    for (int size : ARRAY_SIZES)
    {
        std::vector<double> staticAccessTimes;

        for (int i = 0; i < NUM_TESTS; ++i)
        {
            staticAccessTimes.push_back(measureStaticMemoryAccess(size));
        }

        removeOutliers(staticAccessTimes, threshold);

        if (!staticAccessTimes.empty())
        {
            double staticAverage = calculateAverage(staticAccessTimes);
            double staticStdDev = calculateStandardDeviation(staticAccessTimes, staticAverage);
            saveResultsToJSON("C++_static_access.json", staticAverage, staticStdDev, "Static Memory Access", numTests, staticAccessTimes.size(), language, size, threshold, 0);
        }
        else
        {
            std::cout << "All static memory access times were outliers for array size " << size << ".\n";
        }
    }
}

void DynamicAccessMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_dynamic_access.json") << "[]";

    for (int size : ARRAY_SIZES)
    {
        std::vector<double> dynamicAccessTimes;

        for (int i = 0; i < NUM_TESTS; ++i)
        {
            dynamicAccessTimes.push_back(measureDynamicMemoryAccess(size));
        }

        removeOutliers(dynamicAccessTimes, threshold);

        if (!dynamicAccessTimes.empty())
        {
            double dynamicAverage = calculateAverage(dynamicAccessTimes);
            double dynamicStdDev = calculateStandardDeviation(dynamicAccessTimes, dynamicAverage);
            saveResultsToJSON("C++_dynamic_access.json", dynamicAverage, dynamicStdDev, "Dynamic Memory Access", numTests, dynamicAccessTimes.size(), language, size, threshold, 0);
        }
        else
        {
            std::cout << "All dynamic memory access times were outliers for array size " << size << ".\n";
        }
    }
}

void AllocationMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_allocation.json") << "[]";

    for (int size : ARRAY_SIZES)
    {
        std::vector<double> allocTimes;

        for (int i = 0; i < NUM_TESTS; ++i)
        {
            allocTimes.push_back(measureMemoryAllocation(size));
        }

        removeOutliers(allocTimes, threshold);

        if (!allocTimes.empty())
        {
            double allocAverage = calculateAverage(allocTimes);
            double allocStdDev = calculateStandardDeviation(allocTimes, allocAverage);
            saveResultsToJSON("C++_allocation.json", allocAverage, allocStdDev, "Memory Allocation", numTests, allocTimes.size(), language, size, threshold, 0);
        }
        else
        {
            std::cout << "All memory allocation times were outliers for array size " << size << ".\n";
        }
    }
}

void DeallocationMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_deallocation.json") << "[]";

    for (int size : ARRAY_SIZES)
    {
        std::vector<double> deallocTimes;

        for (int i = 0; i < NUM_TESTS; ++i)
        {
            deallocTimes.push_back(measureMemoryDeallocation(size));
        }

        removeOutliers(deallocTimes, threshold);

        if (!deallocTimes.empty())
        {
            double deallocAverage = calculateAverage(deallocTimes);
            double deallocStdDev = calculateStandardDeviation(deallocTimes, deallocAverage);
            saveResultsToJSON("C++_deallocation.json", deallocAverage, deallocStdDev, "Memory Deallocation", numTests, deallocTimes.size(), language, size, threshold, 0);
        }
        else
        {
            std::cout << "All memory deallocation times were outliers for array size " << size << ".\n";
        }
    }
}

void ThreadCreationMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_thread_creation.json") << "[]";

    for (int iterations : ITERATIONS)
    {
        std::vector<double> threadCreationTimes;
        for (int i = 0; i < NUM_TESTS; ++i)
        {
            threadCreationTimes.push_back(measureThreadCreationTime(iterations));
        }

        removeOutliers(threadCreationTimes, threshold);

        if (!threadCreationTimes.empty())
        {
            double threadCreationAverage = calculateAverage(threadCreationTimes);
            double threadCreationStdDev = calculateStandardDeviation(threadCreationTimes, threadCreationAverage);
            saveResultsToJSON("C++_thread_creation.json", threadCreationAverage, threadCreationStdDev, "Thread Creation", numTests, threadCreationTimes.size(), language, 0, threshold, iterations);
        }
        else
        {
            std::cout << "All thread creation times were outliers.\n";
        }
    }
}

void ContextSwitchMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_context_switch.json") << "[]";

    for (int iterations : ITERATIONS)
    {
        std::vector<double> contextSwitchTimes;
        for (int i = 0; i < NUM_TESTS; ++i)
        {
            contextSwitchTimes.push_back(measureContextSwitchTime(iterations));
        }

        removeOutliers(contextSwitchTimes, threshold);

        if (!contextSwitchTimes.empty())
        {
            double contextSwitchAverage = calculateAverage(contextSwitchTimes);
            double contextSwitchStdDev = calculateStandardDeviation(contextSwitchTimes, contextSwitchAverage);
            saveResultsToJSON("C++_context_switch.json", contextSwitchAverage, contextSwitchStdDev, "Context Switch", numTests, contextSwitchTimes.size(), language, 0, threshold, iterations);
        }
        else
        {
            std::cout << "All context switch times were outliers.\n";
        }
    }
}

void ThreadMigrationMain(int numTests, double threshold)
{
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("C++_measurements/C++_thread_migration.json") << "[]";

    for (int iterations : ITERATIONS)
    {
        std::vector<double> threadMigrationTimes;
        for (int i = 0; i < NUM_TESTS; ++i)
        {
            threadMigrationTimes.push_back(measureThreadMigrationTime(iterations));
        }

        removeOutliers(threadMigrationTimes, threshold);

        if (!threadMigrationTimes.empty())
        {
            double threadMigrationAverage = calculateAverage(threadMigrationTimes);
            double threadMigrationStdDev = calculateStandardDeviation(threadMigrationTimes, threadMigrationAverage);
            saveResultsToJSON("C++_thread_migration.json", threadMigrationAverage, threadMigrationStdDev, "Thread Migration", numTests, threadMigrationTimes.size(), language, 0, threshold, iterations);
        }
        else
        {
            std::cout << "All thread migration times were outliers.\n";
        }
    }
}

void callAll_Cpp_Benchmarks(int numTests, double threshold)
{

    StaticAccessMain(numTests, threshold);
    DynamicAccessMain(numTests, threshold);
    AllocationMain(numTests, threshold);
    DeallocationMain(numTests, threshold);
    ThreadCreationMain(numTests, threshold);
    ContextSwitchMain(numTests, threshold);
    ThreadMigrationMain(numTests, threshold);

    combineJSONFiles({"C++_static_access.json", "C++_dynamic_access.json", "C++_allocation.json", "C++_deallocation.json", "C++_thread_creation.json", "C++_context_switch.json", "C++_thread_migration.json"}, "C++_results.json");
}

JNIEXPORT void JNICALL Java_JNInterface_callNative_1Cpp_1Benchmark(JNIEnv *env, jobject obj, jint benchmarkType, jint numTests, jdouble threshold)
{
    switch (benchmarkType)
    {
    case 0:
        callAll_Cpp_Benchmarks(numTests, threshold);
        break;
    case 1:
        StaticAccessMain(numTests, threshold);
        break;
    case 2:
        DynamicAccessMain(numTests, threshold);
        break;
    case 3:
        AllocationMain(numTests, threshold);
        break;
    case 4:
        DeallocationMain(numTests, threshold);
        break;
    case 5:
        ThreadCreationMain(numTests, threshold);
        break;
    case 6:
        ContextSwitchMain(numTests, threshold);
        break;
    case 7:
        ThreadMigrationMain(numTests, threshold);
        break;
    default:
        std::cerr << "Invalid benchmark type" << std::endl;
        break;
    }
}
