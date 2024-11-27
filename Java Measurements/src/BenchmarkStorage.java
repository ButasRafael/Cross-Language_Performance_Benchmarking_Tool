import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class BenchmarkStorage {
    private final List<BenchmarkResult> results;

    public BenchmarkStorage() {
        this.results = new ArrayList<>();
    }

    public void loadFromJsonFile(String filePath) {
        try {
            String content = new String(Files.readAllBytes(Paths.get(filePath)));

            JSONArray jsonArray = new JSONArray(content);

            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject obj = jsonArray.getJSONObject(i);

                BenchmarkResult result = new BenchmarkResult(
                        obj.optInt("array_size", 0),
                        obj.optInt("iterations", 0),
                        obj.getInt("number_of_tests"),
                        obj.getInt("passed_tests"),
                        obj.getInt("outlier_threshold"),
                        obj.getString("programming_language"),
                        obj.getString("process_measured"),
                        obj.getDouble("average_time"),
                        obj.getDouble("std_deviation")
                );

                results.add(result);
            }

        } catch (IOException e) {
            System.err.println("Error reading JSON file: " + e.getMessage());
        }
    }

    public List<BenchmarkResult> getResults() {
        return results;
    }

    public void printResults() {
        for (BenchmarkResult result : results) {
            System.out.println(result);
        }
    }
}
