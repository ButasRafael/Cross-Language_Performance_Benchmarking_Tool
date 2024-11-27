import org.json.JSONObject;
import org.json.JSONArray;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class BenchmarkEngine {
    private JNInterface jni;
    public BenchmarkEngine() {
        jni = new JNInterface();
    }

    private static final int[] ARRAY_SIZES = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
    private static final int[] ITERATIONS= {2, 10, 100, 1000, 10000};
    private static final String LANGUAGE = "Java";
    private static final JSONArray allocationResults = new JSONArray();
    private static final JSONArray deallocationResults = new JSONArray();
    private static final JSONArray staticAccessResults = new JSONArray();
    private static final JSONArray dynamicAccessResults = new JSONArray();
    private static final JSONArray threadCreationResults = new JSONArray();
    private static final JSONArray contextSwitchResults = new JSONArray();
    private static final JSONArray threadMigrationResults = new JSONArray();

    private static double calculateAverage(double[] times, int size) {
        return Arrays.stream(times, 0, size).average().orElse(0.0);
    }

    private static double calculateStandardDeviation(double[] times, double mean, int size) {
        double variance = Arrays.stream(times, 0, size).map(t -> (t - mean) * (t - mean)).sum() / size;
        return Math.sqrt(variance);
    }

    private static int removeOutliers(double[] times, double threshold) {
        double mean = calculateAverage(times, times.length);
        double stdDev = calculateStandardDeviation(times, mean, times.length);
        double lowerThreshold = mean - threshold * stdDev;
        double upperThreshold = mean + threshold * stdDev;

        int newSize = 0;
        for (int i = 0; i < times.length; i++) {
            if (times[i] >= lowerThreshold && times[i] <= upperThreshold) {
                times[newSize++] = times[i];
            }
        }
        return newSize;
    }

    private static double measureStaticMemoryAccess(int size) {
        final int[] staticArray = new int[size];
        for (int i = 0; i < size; i++) {
            staticArray[i] = i;
        }

        long startTime = System.nanoTime();
        int sum = 0;
        for (int i = 0; i < size; i++) {
            sum += staticArray[i];
        }
        long endTime = System.nanoTime();

        return (endTime - startTime) / (double) size;
    }

    private static double measureDynamicMemoryAccess(int size) {
        int[] dynamicArray = new int[size];
        for (int i = 0; i < size; i++) {
            dynamicArray[i] = i;
        }

        long startTime = System.nanoTime();
        int sum = 0;
        for (int i = 0; i < size; i++) {
            sum += dynamicArray[i];
        }
        long endTime = System.nanoTime();

        return (endTime - startTime) / (double) size;
    }

    private static double measureMemoryAllocation(int size) {
        long startTime = System.nanoTime();
        int[][] chunks = new int[size][];
        for (int i = 0; i < size; i++) {
            chunks[i] = new int[1];
        }
        long endTime = System.nanoTime();

        for (int i = 0; i < size; i++) {
            chunks[i] = null;
        }
        return (endTime - startTime) / (double) size;
    }

    private static double measureMemoryDeallocation(int size) {
        int[][] chunks = new int[size][];
        for (int i = 0; i < size; i++) {
            chunks[i] = new int[1];
        }

        long startTime = System.nanoTime();
        for (int i = 0; i < size; i++) {
            chunks[i] = null;
        }
        System.gc();
        long endTime = System.nanoTime();

        return (endTime - startTime) / (double) size;
    }

    private static double measureThreadCreationTime(int iterations) throws InterruptedException {
        long startTime = System.nanoTime();
        for (int i = 0; i < iterations; i++) {
            Thread thread = new Thread(() -> {
                int sum = 0;
                for (int j = 0; j < 1000; j++) {
                    sum += j;
                }
            });
            thread.start();
            thread.join();
        }
        long endTime = System.nanoTime();

        return (endTime - startTime) / (double) iterations;
    }

    private static double measureContextSwitchTime(int iterations) throws InterruptedException {
        Lock lock = new ReentrantLock();
        Condition condition1 = lock.newCondition();
        Condition condition2 = lock.newCondition();
        AtomicBoolean turn = new AtomicBoolean(true);

        Runnable thread1Task = () -> {
            try {
                for (int i = 0; i < iterations / 2; i++) {
                    lock.lock();
                    try {
                        while (!turn.get()) {
                            condition1.await();
                        }
                        turn.set(false);
                        condition2.signal();
                    } finally {
                        lock.unlock();
                    }
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        };

        Runnable thread2Task = () -> {
            try {
                for (int i = 0; i < iterations / 2; i++) {
                    lock.lock();
                    try {
                        while (turn.get()) {
                            condition2.await();
                        }
                        turn.set(true);
                        condition1.signal();
                    } finally {
                        lock.unlock();
                    }
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        };

        long startTime = System.nanoTime();

        Thread t1 = new Thread(thread1Task);
        Thread t2 = new Thread(thread2Task);
        t1.start();
        t2.start();
        t1.join();
        t2.join();

        long endTime = System.nanoTime();
        return (endTime - startTime) / (double) iterations;
    }

    static {
        System.loadLibrary("mynative_java");
    }

    public native double measureThreadMigrationTime(int iterations);

    private static void saveResultToArray(JSONArray resultsArray, double average, double stdDev, String process, int numTests, int passedTests, String language, int arraySize, int iterations, double threshold, String fileName) {
        File directory = new File("Java_measurements");
        if (!directory.exists()) {
            directory.mkdir();
        }

        File file = new File(directory, fileName);

        JSONObject json = new JSONObject();
        if (arraySize > 0) {
            json.put("array_size", arraySize);
        }
        if (iterations > 0) {
            json.put("iterations", iterations);
        }
        json.put("number_of_tests", numTests);
        json.put("passed_tests", passedTests);
        json.put("outlier_threshold", threshold);
        json.put("programming_language", language);
        json.put("process_measured", process);
        json.put("average_time", average);
        json.put("std_deviation", stdDev);
        resultsArray.put(json);

        try (FileWriter fileWriter = new FileWriter(file)) {
            fileWriter.write(resultsArray.toString(4));
            fileWriter.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    private static void createCombinedResultsJSON() {

        JSONArray combinedResults = new JSONArray();
        File directory = new File("Java_measurements");
        combinedResults.putAll(allocationResults);
        combinedResults.putAll(deallocationResults);
        combinedResults.putAll(staticAccessResults);
        combinedResults.putAll(dynamicAccessResults);
        combinedResults.putAll(threadCreationResults);
        combinedResults.putAll(contextSwitchResults);
        combinedResults.putAll(threadMigrationResults);

        File file = new File(directory, "Java_results.json");
        try (FileWriter fileWriter = new FileWriter(file)) {
            fileWriter.write(combinedResults.toString(4));
            fileWriter.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void measureStaticMemoryAccess(int numTests, double threshold) {
        for (int size : ARRAY_SIZES) {
            double[] staticAccessTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                staticAccessTimes[i] = measureStaticMemoryAccess(size);
            }
            int staticSize = removeOutliers(staticAccessTimes, threshold);
            if (staticSize > 0) {
                double average = calculateAverage(staticAccessTimes, staticSize);
                double stdDev = calculateStandardDeviation(staticAccessTimes, average, staticSize);
                saveResultToArray(staticAccessResults, average, stdDev, "Static Memory Access", numTests, staticSize, LANGUAGE, size,0,threshold, "Java_static_access.json");
            } else {
                System.out.println("All static memory access times are outliers for array size: " + size);
            }
        }
    }

    private static void measureDynamicMemoryAccess(int numTests, double threshold) {
        for (int size : ARRAY_SIZES) {
            double[] dynamicAccessTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                dynamicAccessTimes[i] = measureDynamicMemoryAccess(size);
            }
            int dynamicSize = removeOutliers(dynamicAccessTimes, threshold);
            if (dynamicSize > 0) {
                double average = calculateAverage(dynamicAccessTimes, dynamicSize);
                double stdDev = calculateStandardDeviation(dynamicAccessTimes, average, dynamicSize);
                saveResultToArray(dynamicAccessResults, average, stdDev, "Dynamic Memory Access", numTests, dynamicSize, LANGUAGE, size,0, threshold, "Java_dynamic_access.json");
            } else {
                System.out.println("All dynamic memory access times are outliers for array size: " + size);
            }
        }
    }

    private static void measureMemoryAllocation(int numTests, double threshold) {
        for (int size : ARRAY_SIZES) {
            double[] allocTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                allocTimes[i] = measureMemoryAllocation(size);
            }
            int allocSize = removeOutliers(allocTimes, threshold);
            if (allocSize > 0) {
                double average = calculateAverage(allocTimes, allocSize);
                double stdDev = calculateStandardDeviation(allocTimes, average, allocSize);
                saveResultToArray(allocationResults, average, stdDev, "Memory Allocation", numTests, allocSize, LANGUAGE, size, 0,threshold, "Java_allocation.json");
            } else {
                System.out.println("All memory allocation times are outliers for array size: " + size);
            }
        }
    }

    private static void measureMemoryDeallocation(int numTests, double threshold) {
        for (int size : ARRAY_SIZES) {
            double[] deallocTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                deallocTimes[i] = measureMemoryDeallocation(size);
            }
            int deallocSize = removeOutliers(deallocTimes, threshold);
            if (deallocSize > 0) {
                double average = calculateAverage(deallocTimes, deallocSize);
                double stdDev = calculateStandardDeviation(deallocTimes, average, deallocSize);
                saveResultToArray(deallocationResults, average, stdDev, "Memory Deallocation", numTests, deallocSize, LANGUAGE, size, 0,threshold, "Java_deallocation.json");
            } else {
                System.out.println("All memory deallocation times are outliers for array size: " + size);
            }
        }
    }

    private static void measureThreadCreation(int numTests, double threshold) throws InterruptedException {
        for (int iterations : ITERATIONS) {
            double[] threadCreationTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                threadCreationTimes[i] = measureThreadCreationTime(iterations);
            }
            int creationSize = removeOutliers(threadCreationTimes, threshold);
            if (creationSize > 0) {
                double average = calculateAverage(threadCreationTimes, creationSize);
                double stdDev = calculateStandardDeviation(threadCreationTimes, average, creationSize);
                saveResultToArray(threadCreationResults, average, stdDev, "Thread Creation", numTests, creationSize, LANGUAGE, 0, iterations, threshold, "Java_thread_creation.json");
            } else {
                System.out.println("All thread creation times are outliers for iterations: " + iterations);
            }
        }
    }

    private static void measureContextSwitch(int numTests, double threshold) throws InterruptedException {
        for (int iterations : ITERATIONS) {
            double[] contextSwitchTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                contextSwitchTimes[i] = measureContextSwitchTime(iterations);
            }
            int switchSize = removeOutliers(contextSwitchTimes, threshold);
            if (switchSize > 0) {
                double average = calculateAverage(contextSwitchTimes, switchSize);
                double stdDev = calculateStandardDeviation(contextSwitchTimes, average, switchSize);
                saveResultToArray(contextSwitchResults, average, stdDev, "Context Switch", numTests, switchSize, LANGUAGE, 0, iterations, threshold, "Java_context_switch.json");
            } else {
                System.out.println("All context switch times are outliers for iterations: " + iterations);
            }
        }
    }

    private static void measureThreadMigration(int numTests, double threshold) {
        BenchmarkEngine performanceMeasurement = new BenchmarkEngine();
        for (int iterations : ITERATIONS) {
            double[] migrationTimes = new double[numTests];
            for (int i = 0; i < numTests; i++) {
                migrationTimes[i] = performanceMeasurement.measureThreadMigrationTime(iterations);
            }
            int migrationSize = removeOutliers(migrationTimes, threshold);
            if (migrationSize > 0) {
                double average = calculateAverage(migrationTimes, migrationSize);
                double stdDev = calculateStandardDeviation(migrationTimes, average, migrationSize);
                saveResultToArray(threadMigrationResults, average, stdDev, "Thread Migration", numTests, migrationSize, LANGUAGE, 0, iterations, threshold, "Java_thread_migration.json");
            } else {
                System.out.println("All thread migration times are outliers for iterations: " + iterations);
            }
        }
    }

    public static void runAllJavaBenchmarks(int numTests, double threshold) throws InterruptedException {
        measureStaticMemoryAccess(numTests, threshold);
        measureDynamicMemoryAccess(numTests, threshold);
        measureMemoryAllocation(numTests, threshold);
        measureMemoryDeallocation(numTests, threshold);
        measureThreadCreation(numTests, threshold);
        measureContextSwitch(numTests, threshold);
        measureThreadMigration(numTests, threshold);

        createCombinedResultsJSON();
    }

    private static void callJavaBenchmark(int benchmarkType, int numTests, double threshold) throws InterruptedException {
        switch (benchmarkType) {
            case 0:
                runAllJavaBenchmarks(numTests, threshold);
                break;
            case 1:
                measureStaticMemoryAccess(numTests, threshold);
                break;
            case 2:
                measureDynamicMemoryAccess(numTests, threshold);
                break;
            case 3:
                measureMemoryAllocation(numTests, threshold);
                break;
            case 4:
                measureMemoryDeallocation(numTests, threshold);
                break;
            case 5:
                measureThreadCreation(numTests, threshold);
                break;
            case 6:
                measureContextSwitch(numTests, threshold);
                break;
            case 7:
                measureThreadMigration(numTests, threshold);
                break;
            default:
                System.out.println("Invalid benchmark type");
        }
    }


    public void runBenchmark(String language, int benchmarkType, int numTests, double threshold) throws InterruptedException {
        switch (language.toLowerCase()) {
            case "c":
                jni.callNative_C_Benchmark(benchmarkType, numTests, threshold);
                break;
            case "c++":
                jni.callNative_Cpp_Benchmark(benchmarkType, numTests, threshold);
                break;
            case "java":
                callJavaBenchmark(benchmarkType, numTests, threshold);
                break;
            default:
                throw new IllegalArgumentException("Unsupported language: " + language);
        }
    }



}