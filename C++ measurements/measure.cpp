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

using ordered_json = nlohmann::ordered_json;

const int NUM_TESTS = 100;
const std::vector<int> ARRAY_SIZES = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
const int CONTEXT_SWITCH_ITERATIONS = 10000;

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
        sum+=staticArray[i];
    }
    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time;
    // return time / size;
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
        sum+=dynamicArray[i];
    }
    auto end = std::chrono::high_resolution_clock::now();

    delete[] dynamicArray;
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time;
    // return time / size;
}

double measureMemoryAllocation(int size)
{
    auto start = std::chrono::high_resolution_clock::now();
    int *array = new int[size];
    auto end = std::chrono::high_resolution_clock::now();
    delete[] array;
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time;
    // return time / size;
}

double measureMemoryDeallocation(int size)
{
    int *array = new int[size];
    auto start = std::chrono::high_resolution_clock::now();
    delete[] array;
    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time;
    // return time / size;
}

double measureThreadCreationTime()
{
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t([] {});
    auto end = std::chrono::high_resolution_clock::now();
    t.join();
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    return time;
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
    double time = std::chrono::duration<double, std::nano>(end - start).count();
    std::cout << "Switches: " << switches.load() << std::endl;
    return time / switches.load();
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

    return std::chrono::duration<double, std::nano>(end - start).count();
}

void saveResultsToJSON(const std::string &filename, double average, double stdDev, const std::string &process, int numTests, int passedTests, const std::string &language, int arraySize, double threshold)
{
    ordered_json json;
    std::ifstream file_in(filename);
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
    result["number_of_tests"] = numTests;
    result["passed_tests"] = passedTests;
    result["outlier_threshold"] = threshold;
    result["programming_language"] = language;
    result["process_measured"] = process;
    result["average_time"] = average;
    result["std_deviation"] = stdDev;

    json.push_back(result);

    std::ofstream file_out(filename);
    file_out << json.dump(4);
    file_out.flush();
    file_out.close();
}

void combineJSONFiles(const std::vector<std::string> &filenames, const std::string &outputFilename)
{
    ordered_json combinedResults = ordered_json::array();

    for (const auto &filename : filenames)
    {
        std::ifstream inputFile(filename);
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

    std::ofstream outputFile(outputFilename);
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

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <number_of_tests> <outlier_threshold>\n";
        return 1;
    }

    int numTests = std::stoi(argv[1]);
    double threshold = std::stod(argv[2]);
    const char language[] = "C++";
    std::cout << std::fixed << std::setprecision(6);

    std::ofstream("static_access.json") << "[]";
    std::ofstream("dynamic_access.json") << "[]";
    std::ofstream("allocation.json") << "[]";
    std::ofstream("deallocation.json") << "[]";
    std::ofstream("thread_creation.json") << "[]";
    std::ofstream("context_switch.json") << "[]";
    std::ofstream("thread_migration.json") << "[]";

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

        removeOutliers(staticAccessTimes, threshold);
        removeOutliers(dynamicAccessTimes, threshold);
        removeOutliers(allocTimes, threshold);
        removeOutliers(deallocTimes, threshold);

        if (!staticAccessTimes.empty())
        {
            double staticAverage = calculateAverage(staticAccessTimes);
            double staticStdDev = calculateStandardDeviation(staticAccessTimes, staticAverage);
            saveResultsToJSON("static_access.json", staticAverage, staticStdDev, "Static Memory Access", numTests, staticAccessTimes.size(), language, size, threshold);
        }
        else
        {
            std::cout << "All static memory access times were outliers for array size " << size << ".\n";
        }

        if (!dynamicAccessTimes.empty())
        {
            double dynamicAverage = calculateAverage(dynamicAccessTimes);
            double dynamicStdDev = calculateStandardDeviation(dynamicAccessTimes, dynamicAverage);
            saveResultsToJSON("dynamic_access.json", dynamicAverage, dynamicStdDev, "Dynamic Memory Access", numTests, dynamicAccessTimes.size(), language, size, threshold);
        }
        else
        {
            std::cout << "All dynamic memory access times were outliers for array size " << size << ".\n";
        }

        if (!allocTimes.empty())
        {
            double allocAverage = calculateAverage(allocTimes);
            double allocStdDev = calculateStandardDeviation(allocTimes, allocAverage);
            saveResultsToJSON("allocation.json", allocAverage, allocStdDev, "Memory Allocation", numTests, allocTimes.size(), language, size, threshold);
        }
        else
        {
            std::cout << "All memory allocation times were outliers for array size " << size << ".\n";
        }

        if (!deallocTimes.empty())
        {
            double deallocAverage = calculateAverage(deallocTimes);
            double deallocStdDev = calculateStandardDeviation(deallocTimes, deallocAverage);
            saveResultsToJSON("deallocation.json", deallocAverage, deallocStdDev, "Memory Deallocation", numTests, deallocTimes.size(), language, size, threshold);
        }
        else
        {
            std::cout << "All memory deallocation times were outliers for array size " << size << ".\n";
        }
    }

    std::vector<double> threadCreationTimes, contextSwitchTimes, threadMigrationTimes;

    for (int i = 0; i < NUM_TESTS; ++i)
    {
        threadCreationTimes.push_back(measureThreadCreationTime());
        contextSwitchTimes.push_back(measureContextSwitchTime());
        threadMigrationTimes.push_back(measureThreadMigrationTime());
    }

    removeOutliers(threadCreationTimes, threshold);
    removeOutliers(contextSwitchTimes, threshold);
    removeOutliers(threadMigrationTimes, threshold);

    if (!threadCreationTimes.empty())
    {
        double threadCreationAverage = calculateAverage(threadCreationTimes);
        double threadCreationStdDev = calculateStandardDeviation(threadCreationTimes, threadCreationAverage);
        saveResultsToJSON("thread_creation.json", threadCreationAverage, threadCreationStdDev, "Thread Creation", numTests, threadCreationTimes.size(), language, 0, threshold);
    }
    else
    {
        std::cout << "All thread creation times were outliers.\n";
    }

    if (!contextSwitchTimes.empty())
    {
        double contextSwitchAverage = calculateAverage(contextSwitchTimes);
        double contextSwitchStdDev = calculateStandardDeviation(contextSwitchTimes, contextSwitchAverage);
        saveResultsToJSON("context_switch.json", contextSwitchAverage, contextSwitchStdDev, "Context Switch", numTests, contextSwitchTimes.size(), language, 0, threshold);
    }
    else
    {
        std::cout << "All context switch times were outliers.\n";
    }

    if (!threadMigrationTimes.empty())
    {
        double threadMigrationAverage = calculateAverage(threadMigrationTimes);
        double threadMigrationStdDev = calculateStandardDeviation(threadMigrationTimes, threadMigrationAverage);
        saveResultsToJSON("thread_migration.json", threadMigrationAverage, threadMigrationStdDev, "Thread Migration", numTests, threadMigrationTimes.size(), language, 0, threshold);
    }
    else
    {
        std::cout << "All thread migration times were outliers.\n";
    }

    std::vector<std::string> filenames = {
        "static_access.json",
        "dynamic_access.json",
        "allocation.json",
        "deallocation.json",
        "thread_creation.json",
        "context_switch.json",
        "thread_migration.json"};

    std::string outputFilename = "result.json";

    combineJSONFiles(filenames, outputFilename);

    std::cout << "Combined results saved to " << outputFilename << std::endl;

    return 0;
}