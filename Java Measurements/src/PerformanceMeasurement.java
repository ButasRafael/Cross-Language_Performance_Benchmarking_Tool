import org.json.JSONObject;
import org.json.JSONArray;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.atomic.AtomicInteger;

public class PerformanceMeasurement {

    private static final int[] ARRAY_SIZES = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
    private static final String LANGUAGE = "Java";
    private static final int CONTEXT_SWITCH_ITERATIONS = 10000;
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

    private static double measureMemoryAllocation(int size) {
        long startTime = System.nanoTime();
        int[] array = new int[size];
        long endTime = System.nanoTime();
        return (endTime - startTime);
    }

    private static double measureMemoryDeallocation(int size) {
        int[] array = new int[size];
        array = null;
        long startTime = System.nanoTime();
        System.gc();
        long endTime = System.nanoTime();
        return (endTime - startTime);
    }

    private static double measureStaticMemoryAccess(int size) {
        final int[] staticArray = new int[size];
        for (int i = 0; i < size; i++) {
            staticArray[i] = i;
        }

        long startTime = System.nanoTime();
        int sum = 0;
        for (int i = 0; i < size; i++) {
            sum+=staticArray[i];
        }
        long endTime = System.nanoTime();

        return (endTime - startTime);
    }

    private static double measureDynamicMemoryAccess(int size) {
        int[] dynamicArray = new int[size];
        for (int i = 0; i < size; i++) {
            dynamicArray[i] = i;
        }

        long startTime = System.nanoTime();
        int sum = 0;
        for (int i = 0; i < size; i++) {
            sum+=dynamicArray[i];
        }
        long endTime = System.nanoTime();

        return (endTime - startTime);
    }

    private static double measureThreadCreationTime() throws InterruptedException {
        long startTime = System.nanoTime();
        Thread thread = new Thread(() -> {});
        thread.start();
        long endTime = System.nanoTime();
        thread.join();
        return (endTime - startTime);
    }

    private static double measureContextSwitchTime() throws InterruptedException {
        Lock lock = new ReentrantLock();
        Condition condition1 = lock.newCondition();
        Condition condition2 = lock.newCondition();
        AtomicInteger switches = new AtomicInteger(0);
        AtomicBoolean turn = new AtomicBoolean(true);

        Runnable thread1Task = () -> {
            try {
                for (int i = 0; i < CONTEXT_SWITCH_ITERATIONS / 2; i++) {
                    lock.lock();
                    try {
                        while (!turn.get()) {
                            condition1.await();
                        }
                        turn.set(false);
                        switches.incrementAndGet();
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
                for (int i = 0; i < CONTEXT_SWITCH_ITERATIONS / 2; i++) {
                    lock.lock();
                    try {
                        while (turn.get()) {
                            condition2.await();
                        }
                        turn.set(true);
                        switches.incrementAndGet();
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
        double timePerSwitch = (endTime - startTime) / (double) switches.get();
        System.out.println("Switches: " + switches.get());
        return timePerSwitch;
    }

    static {
        System.loadLibrary("mynative");
    }

    public native double measureThreadMigrationTime();

    private static void saveResultToArray(JSONArray resultsArray, double average, double stdDev, String process, int numTests, int passedTests, String language, int arraySize, double threshold,String fileName) {
        JSONObject json = new JSONObject();
        if(arraySize > 0) {
            json.put("array_size", arraySize);
        }

        json.put("number_of_tests", numTests);
        json.put("passed_tests", passedTests);
        json.put("outlier_threshold", threshold);
        json.put("programming_language", language);
        json.put("process_measured", process);
        json.put("average_time", average);
        json.put("std_deviation", stdDev);
        resultsArray.put(json);
        try (FileWriter file = new FileWriter(fileName)) {
            file.write(resultsArray.toString(4));
            file.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    private static void createCombinedResultsJSON() {
        JSONArray combinedResults = new JSONArray();

        combinedResults.putAll(allocationResults);
        combinedResults.putAll(deallocationResults);
        combinedResults.putAll(staticAccessResults);
        combinedResults.putAll(dynamicAccessResults);
        combinedResults.putAll(threadCreationResults);
        combinedResults.putAll(contextSwitchResults);
        combinedResults.putAll(threadMigrationResults);

        try (FileWriter file = new FileWriter("results.json")) {
            file.write(combinedResults.toString(4));
            file.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    public static void main(String[] args) throws InterruptedException {
        if (args.length != 2) {
            System.err.println("Usage: PerformanceMeasurement <number_of_tests> <outlier_threshold>");
            return;
        }

        int numTests = Integer.parseInt(args[0]);
        double threshold = Double.parseDouble(args[1]);
        PerformanceMeasurement performanceMeasurement = new PerformanceMeasurement();

        for (int size : ARRAY_SIZES) {
            double[] staticAccessTimes = new double[numTests];
            double[] dynamicAccessTimes = new double[numTests];
            double[] allocTimes = new double[numTests];
            double[] deallocTimes = new double[numTests];

            for (int i = 0; i < numTests; i++) {
                staticAccessTimes[i] = measureStaticMemoryAccess(size);
                dynamicAccessTimes[i] = measureDynamicMemoryAccess(size);
                allocTimes[i] = measureMemoryAllocation(size);
                deallocTimes[i] = measureMemoryDeallocation(size);
            }

            int staticSize = removeOutliers(staticAccessTimes, threshold);
            int dynamicSize = removeOutliers(dynamicAccessTimes, threshold);
            int allocSize = removeOutliers(allocTimes, threshold);
            int deallocSize = removeOutliers(deallocTimes, threshold);

            if (staticSize > 0) {
                double average = calculateAverage(staticAccessTimes, staticSize);
                double stdDev = calculateStandardDeviation(staticAccessTimes, average, staticSize);
                saveResultToArray(staticAccessResults, average, stdDev, "Static Memory Access", numTests, staticSize, LANGUAGE, size, threshold,"static_access.json");

            }

            else{
                System.out.println("All static memory access times are outliers for array size: " + size);
            }

            if (dynamicSize > 0) {
                double average = calculateAverage(dynamicAccessTimes, dynamicSize);
                double stdDev = calculateStandardDeviation(dynamicAccessTimes, average, dynamicSize);
                saveResultToArray(dynamicAccessResults, average, stdDev, "Dynamic Memory Access", numTests, dynamicSize, LANGUAGE, size, threshold,"dynamic_access.json");
            }
            else{
                System.out.println("All dynamic memory access times are outliers for array size: " + size);
            }

            if (allocSize > 0) {
                double average = calculateAverage(allocTimes, allocSize);
                double stdDev = calculateStandardDeviation(allocTimes, average, allocSize);
                saveResultToArray(allocationResults, average, stdDev, "Memory Allocation", numTests, allocSize, LANGUAGE, size, threshold,"allocation.json");

            }

                else{
                    System.out.println("All memory allocation times are outliers for array size: " + size);
                }

            if (deallocSize > 0) {
                double average = calculateAverage(deallocTimes, deallocSize);
                double stdDev = calculateStandardDeviation(deallocTimes, average, deallocSize);
                saveResultToArray(deallocationResults, average, stdDev, "Memory Deallocation", numTests, deallocSize, LANGUAGE, size, threshold,"deallocation.json");
            }

            else{
                System.out.println("All memory deallocation times are outliers for array size: " + size);
            }
        }

        double[] threadCreationTimes = new double[numTests];
        double[] contextSwitchTimes = new double[numTests];
        double[] threadMigrationTimes = new double[numTests];
        for (int i = 0; i < numTests; i++) {
            threadCreationTimes[i] = measureThreadCreationTime();
            contextSwitchTimes[i] = measureContextSwitchTime();
            threadMigrationTimes[i]= performanceMeasurement.measureThreadMigrationTime();
        }

        int creationSize = removeOutliers(threadCreationTimes, threshold);
        int switchSize = removeOutliers(contextSwitchTimes, threshold);
        int migrationSize = removeOutliers(threadMigrationTimes, threshold);
        if (creationSize > 0) {
            double average = calculateAverage(threadCreationTimes, creationSize);
            double stdDev = calculateStandardDeviation(threadCreationTimes, average, creationSize);
            saveResultToArray(threadCreationResults, average, stdDev, "Thread Creation", numTests, creationSize, LANGUAGE, 0, threshold,"thread_creation.json");
        }

        else{
            System.out.println("All thread creation times are outliers");
        }

        if (switchSize > 0) {
            double average = calculateAverage(contextSwitchTimes, switchSize);
            double stdDev = calculateStandardDeviation(contextSwitchTimes, average, switchSize);
            saveResultToArray(contextSwitchResults, average, stdDev, "Context Switch", numTests, switchSize, LANGUAGE, 0, threshold,"context_switch.json");
        }

        else{
            System.out.println("All context switch times are outliers");
        }

        if (migrationSize > 0) {
            double average = calculateAverage(threadMigrationTimes, migrationSize);
            double stdDev = calculateStandardDeviation(threadMigrationTimes, average, migrationSize);
            saveResultToArray(threadMigrationResults, average, stdDev, "Thread Migration", numTests, migrationSize, LANGUAGE, 0, threshold,"thread_migration.json");
        }

        else{
            System.out.println("All thread migration times are outliers");
        }

        createCombinedResultsJSON();
    }
}