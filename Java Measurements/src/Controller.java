public class Controller {
    private BenchmarkEngine benchmarkEngine;
    private BenchmarkStorage benchmarkStorage;
    private GraphGenerator graphGenerator;

    public Controller() {
        this.benchmarkEngine = new BenchmarkEngine();
        this.benchmarkStorage = new BenchmarkStorage();
        this.graphGenerator = new GraphGenerator();
    }

    public void runBenchmark(String language, int benchmarkType, int numTests, double threshold) throws InterruptedException {
        benchmarkEngine.runBenchmark(language, benchmarkType, numTests, threshold);
    }

    public void loadResults(String language, int benchmarkType) {
        String filePath = getBenchmarkFilePath(language, benchmarkType);
        if (filePath != null) {
            benchmarkStorage.loadFromJsonFile(filePath);
        } else {
            System.out.println("Invalid benchmark type");
        }
    }

    private String getBenchmarkFilePath(String language, int benchmarkType) {
        switch (benchmarkType) {
            case 0: return language + "_measurements/" + language + "_results.json";
            case 1: return language + "_measurements/" + language + "_static_access.json";
            case 2: return language + "_measurements/" + language + "_dynamic_access.json";
            case 3: return language + "_measurements/" + language + "_allocation.json";
            case 4: return language + "_measurements/" + language + "_deallocation.json";
            case 5: return language + "_measurements/" + language + "_thread_creation.json";
            case 6: return language + "_measurements/" + language + "_context_switch.json";
            case 7: return language + "_measurements/" + language + "_thread_migration.json";
            default: return null;
        }
    }

    public void generateGraph(String measurement) {
        graphGenerator.createAndShowGraph(measurement, benchmarkStorage);
    }

    public BenchmarkStorage getBenchmarkStorage() {
        return benchmarkStorage;
    }

    public void printResults() {
        benchmarkStorage.printResults();
    }
}