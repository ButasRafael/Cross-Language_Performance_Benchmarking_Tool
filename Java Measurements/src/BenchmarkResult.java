public class BenchmarkResult {
    private final int arraySize;
    private final int iterations;
    private final int numberOfTests;
    private final int passedTests;
    private final int outlierThreshold;
    private final String programmingLanguage;
    private final String processMeasured;
    private final double averageTime;
    private final double stdDeviation;

    // Constructor
    public BenchmarkResult(int arraySize, int iterations,int numberOfTests, int passedTests, int outlierThreshold,
                           String programmingLanguage, String processMeasured, double averageTime, double stdDeviation) {
        this.arraySize = arraySize;
        this.iterations = iterations;
        this.numberOfTests = numberOfTests;
        this.passedTests = passedTests;
        this.outlierThreshold = outlierThreshold;
        this.programmingLanguage = programmingLanguage;
        this.processMeasured = processMeasured;
        this.averageTime = averageTime;
        this.stdDeviation = stdDeviation;
    }

    // Getters
    public int getArraySize() {
        return arraySize;
    }

    public int getIterations() {
        return iterations;
    }

    public int getNumberOfTests() {
        return numberOfTests;
    }

    public int getPassedTests() {
        return passedTests;
    }

    public int getOutlierThreshold() {
        return outlierThreshold;
    }

    public String getProgrammingLanguage() {
        return programmingLanguage;
    }

    public String getProcessMeasured() {
        return processMeasured;
    }

    public double getAverageTime() {
        return averageTime;
    }

    public double getStdDeviation() {
        return stdDeviation;
    }

    @Override
    public String toString() {
        return "BenchmarkResult{" +
                "arraySize=" + arraySize +
                ", numberOfTests=" + numberOfTests +
                ", passedTests=" + passedTests +
                ", outlierThreshold=" + outlierThreshold +
                ", programmingLanguage='" + programmingLanguage + '\'' +
                ", processMeasured='" + processMeasured + '\'' +
                ", averageTime=" + averageTime +
                ", stdDeviation=" + stdDeviation +
                '}';
    }
}

