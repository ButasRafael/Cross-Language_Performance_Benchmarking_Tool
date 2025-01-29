import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.ChartUtils;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.axis.CategoryAxis;
import org.jfree.chart.labels.CategoryToolTipGenerator;
import org.jfree.chart.plot.CategoryPlot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.renderer.category.BarRenderer;
import org.jfree.chart.renderer.category.LineAndShapeRenderer;
import org.jfree.chart.title.LegendTitle;
import org.jfree.chart.ui.RectangleEdge;
import org.jfree.data.category.DefaultCategoryDataset;

import javax.swing.*;
import java.awt.*;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class GraphGenerator {

    public void createAndShowGraph(String measurement, BenchmarkStorage benchmarkStorage) {
        DefaultCategoryDataset dataset = createDataset(measurement, benchmarkStorage, false);
        JFreeChart lineChart = createLineChart(dataset, measurement);
        JFreeChart barChart = createBarChart(dataset, measurement);
        displayChart(lineChart, barChart,dataset, measurement, benchmarkStorage);
    }


    private DefaultCategoryDataset createDataset(String measurement, BenchmarkStorage benchmarkStorage, boolean showStandardDeviation) {
        DefaultCategoryDataset dataset = new DefaultCategoryDataset();
        addDataToDataset(dataset, benchmarkStorage.getResults(), measurement, showStandardDeviation);
        return dataset;
    }

    private void addDataToDataset(DefaultCategoryDataset dataset, List<BenchmarkResult> results, String measurement, boolean showStandardDeviation) {
        for (BenchmarkResult result : results) {
            if (result.getProcessMeasured().equals(measurement)) {
                String xValue = (measurement.contains("Thread") || measurement.contains("Context"))
                        ? String.valueOf(result.getIterations())
                        : String.valueOf(result.getArraySize());
                double value = showStandardDeviation ? result.getStdDeviation() : result.getAverageTime();
                dataset.addValue(value, result.getProgrammingLanguage(), xValue);
            }
        }
    }
    private JFreeChart createLineChart(DefaultCategoryDataset dataset, String measurement) {
        JFreeChart chart = ChartFactory.createLineChart(
                measurement + " Comparison",
                measurement.contains("Thread") || measurement.contains("Context") ? "Iterations" : "Array Size",
                "Average Time (ns)",
                dataset,
                PlotOrientation.VERTICAL,
                true,
                true,
                false
        );

        CategoryPlot plot = chart.getCategoryPlot();
        LineAndShapeRenderer renderer = new LineAndShapeRenderer();
        renderer.setSeriesShapesVisible(0, true);
        renderer.setSeriesShapesVisible(1, true);
        plot.setRenderer(renderer);

        styleChart(chart, plot);
        return chart;
    }

    private JFreeChart createBarChart(DefaultCategoryDataset dataset, String measurement) {
        JFreeChart chart = ChartFactory.createBarChart(
                measurement + " Comparison",
                measurement.contains("Thread") || measurement.contains("Context") ? "Iterations" : "Array Size",
                "Average Time (ns)",
                dataset,
                PlotOrientation.VERTICAL,
                true,
                true,
                false
        );

        CategoryPlot plot = chart.getCategoryPlot();
        BarRenderer renderer = new BarRenderer();

        renderer.setDefaultToolTipGenerator((dataset1, row, column) -> {
            String series = (String) dataset1.getRowKey(row);
            String category = (String) dataset1.getColumnKey(column);
            Number value = dataset1.getValue(row, column);
            return String.format("<html>Language: %s<br>Category: %s<br>Value: %.2f</html>", series, category, value.doubleValue());
        });

        plot.setRenderer(renderer);
        styleChart(chart, plot);
        return chart;
    }


    private void styleChart(JFreeChart chart, CategoryPlot plot) {
        plot.setBackgroundPaint(new GradientPaint(0, 0, Color.WHITE, 0, 600, Color.LIGHT_GRAY, true));
        plot.setDomainGridlinePaint(Color.GRAY);
        plot.setRangeGridlinePaint(Color.GRAY);

        CategoryAxis domainAxis = plot.getDomainAxis();
        domainAxis.setTickLabelFont(new Font("Arial", Font.PLAIN, 12));
        domainAxis.setLabelFont(new Font("Arial", Font.BOLD, 14));

        ValueAxis rangeAxis = plot.getRangeAxis();
        rangeAxis.setTickLabelFont(new Font("Arial", Font.PLAIN, 12));
        rangeAxis.setLabelFont(new Font("Arial", Font.BOLD, 14));

        chart.getTitle().setFont(new Font("Arial", Font.BOLD, 18));

        LegendTitle legend = chart.getLegend();
        legend.setPosition(RectangleEdge.BOTTOM);
        legend.setItemFont(new Font("Arial", Font.PLAIN, 12));
    }
    private void displayChart(JFreeChart lineChart, JFreeChart barChart,DefaultCategoryDataset dataset, String measurement, BenchmarkStorage benchmarkStorage) {
        JFrame frame = new JFrame(measurement + " Chart");
        frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);

        ChartPanel panel = new ChartPanel(lineChart);
        panel.setMouseWheelEnabled(true);
        panel.setPreferredSize(new Dimension(1000, 800));

        CategoryPlot plot = lineChart.getCategoryPlot();
        LineAndShapeRenderer renderer = (LineAndShapeRenderer) plot.getRenderer();

        renderer.setDefaultToolTipGenerator((CategoryToolTipGenerator) (dataset1, row, column) -> {
            String series = (String) dataset1.getRowKey(row);
            String category = (String) dataset1.getColumnKey(column);
            Number value = dataset1.getValue(row, column);
            return String.format("<html>Language: %s<br>Category: %s<br>Value: %.2f</html>", series, category, value.doubleValue());
        });

        final JFreeChart[] currentChart = {lineChart};

        JComboBox<String> chartTypeSelector = new JComboBox<>(new String[]{"Line Chart", "Bar Chart"});
        chartTypeSelector.addActionListener(e -> {
            currentChart[0] = chartTypeSelector.getSelectedItem().equals("Line Chart") ? lineChart : barChart;
            panel.setChart(currentChart[0]);
        });

        JButton exportButton = new JButton("Export");
        exportButton.addActionListener(e -> {
            String[] options = {"PNG", "CSV"};
            int choice = JOptionPane.showOptionDialog(
                    frame,
                    "Choose export format:",
                    "Export",
                    JOptionPane.DEFAULT_OPTION,
                    JOptionPane.INFORMATION_MESSAGE,
                    null,
                    options,
                    options[0]
            );

            if (choice == 0 || choice == 1) {
                JFileChooser fileChooser = new JFileChooser();
                if (fileChooser.showSaveDialog(frame) == JFileChooser.APPROVE_OPTION) {
                    File file = fileChooser.getSelectedFile();
                    String filePath = file.getAbsolutePath();
                    String extension = choice == 0 ? ".png" : ".csv";

                    if (!filePath.endsWith(extension)) {
                        filePath += extension;
                    }

                    File finalFile = new File(filePath);
                    if (finalFile.exists()) {
                        int overwriteChoice = JOptionPane.showConfirmDialog(
                                frame,
                                "File already exists. Overwrite?",
                                "Confirm Overwrite",
                                JOptionPane.YES_NO_OPTION
                        );
                        if (overwriteChoice != JOptionPane.YES_OPTION) {
                            return; // Cancel operation
                        }
                    }

                    try {
                        if (choice == 0) {
                            ChartUtils.saveChartAsPNG(finalFile, currentChart[0], 1000, 800);
                        } else {
                            try (FileWriter writer = new FileWriter(finalFile)) {
                                for (int row = 0; row < dataset.getRowCount(); row++) {
                                    for (int col = 0; col < dataset.getColumnCount(); col++) {
                                        String language = (String) dataset.getRowKey(row);
                                        String xValue = (String) dataset.getColumnKey(col);
                                        Number value = dataset.getValue(row, col);
                                        writer.write(language + "," + xValue + "," + value + "\n");
                                    }
                                }
                            }
                        }
                        JOptionPane.showMessageDialog(frame, "Export successful!");
                    } catch (IOException ex) {
                        JOptionPane.showMessageDialog(frame, "Failed to export: " + ex.getMessage());
                    }
                }
            }
        });

        final java.util.function.Function<DefaultCategoryDataset, Double> getMaxValue = datasetToAnalyze -> {
            double maxValue = 0;
            for (int row = 0; row < datasetToAnalyze.getRowCount(); row++) {
                if(!renderer.isSeriesVisible(row)) {
                    continue;
                }
                for (int col = 0; col < datasetToAnalyze.getColumnCount(); col++) {
                    Number value = datasetToAnalyze.getValue(row, col);
                    if (value != null && value.doubleValue() > maxValue) {
                        maxValue = value.doubleValue();
                    }
                }
            }
            return maxValue;
        };

        double maxValue = getMaxValue.apply(dataset);
        int sliderMax = (int) Math.ceil(maxValue);
        JSlider rangeSlider = new JSlider(JSlider.HORIZONTAL, 0, sliderMax, sliderMax);
        rangeSlider.addChangeListener(e -> {
            CategoryPlot activePlot = (CategoryPlot) currentChart[0].getPlot();
            activePlot.getRangeAxis().setUpperBound(rangeSlider.getValue());
        });


        JComboBox<String> metricSelector = new JComboBox<>(new String[]{"Average Time", "Standard Deviation"});
        metricSelector.addActionListener(e -> {
            boolean showStandardDeviation = metricSelector.getSelectedItem().equals("Standard Deviation");

            dataset.clear();
            addDataToDataset(dataset, benchmarkStorage.getResults(), measurement, showStandardDeviation);

            String yAxisLabel = showStandardDeviation ? "Standard Deviation" : "Average Time (ns)";
            lineChart.getCategoryPlot().getRangeAxis().setLabel(yAxisLabel);
            barChart.getCategoryPlot().getRangeAxis().setLabel(yAxisLabel);

            double newMaxValue = getMaxValue.apply(dataset);
            rangeSlider.setMaximum((int) Math.ceil(newMaxValue));
            rangeSlider.setValue((int) Math.ceil(newMaxValue));

            currentChart[0].setNotify(true);
        });


        JPanel filterPanel = new JPanel();
        Set<String> languages = new HashSet<>();
        for (BenchmarkResult result : benchmarkStorage.getResults()) {
            languages.add(result.getProgrammingLanguage());
        }
        for (String language : languages) {
            JCheckBox checkBox = new JCheckBox(language, true);
            checkBox.addItemListener(e -> {
                boolean visible = checkBox.isSelected();
                int seriesIndex = dataset.getRowIndex(language);

                LineAndShapeRenderer lineRenderer = (LineAndShapeRenderer) lineChart.getCategoryPlot().getRenderer();
                BarRenderer barRenderer = (BarRenderer) barChart.getCategoryPlot().getRenderer();
                lineRenderer.setSeriesVisible(seriesIndex, visible);
                barRenderer.setSeriesVisible(seriesIndex, visible);

                double newMaxValue = getMaxValue.apply(dataset);
                rangeSlider.setMaximum((int) Math.ceil(newMaxValue));
                rangeSlider.setValue((int) Math.ceil(newMaxValue));

                currentChart[0].setNotify(true);
            });

            filterPanel.add(checkBox);
        }

        JPanel controlPanel = new JPanel();
        controlPanel.add(new JLabel("Select Chart Type:"));
        controlPanel.add(chartTypeSelector);
        controlPanel.add(new JLabel("Select Metric:"));
        controlPanel.add(metricSelector);
        controlPanel.add(new JLabel("Filters:"));
        controlPanel.add(filterPanel);
        controlPanel.add(exportButton);
        controlPanel.add(new JLabel("Adjust Range:"));
        controlPanel.add(rangeSlider);

        JPanel mainPanel = new JPanel(new BorderLayout());
        mainPanel.add(panel, BorderLayout.CENTER);
        mainPanel.add(controlPanel, BorderLayout.SOUTH);

        frame.getContentPane().add(mainPanel);
        frame.setSize(1300, 800);
        frame.setVisible(true);
    }


}