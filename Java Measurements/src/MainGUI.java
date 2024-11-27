import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.HashMap;
import java.util.Map;


public class MainGUI extends JFrame {
    private final Controller controller;
    private JTextArea logArea;
    private JTextField numTestsField;
    private JTextField thresholdField;
    private JProgressBar progressBar;
    private final Map<String, String> benchmarkExplanations;
    private boolean isBenchmarkInProgress = false;

    public MainGUI() {
        controller = new Controller();
        benchmarkExplanations = loadBenchmarkExplanations();
        setupUI();
    }

    private JButton createStyledButton(String text, Color backgroundColor) {
        JButton button = new JButton(text);
        button.setFont(new Font("SansSerif", Font.BOLD, 12));
        button.setBackground(backgroundColor);
        button.setForeground(Color.BLACK);
        button.setFocusPainted(false);
        button.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        button.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseEntered(MouseEvent e) {
                button.setBackground(new Color(135, 206, 250)); // Sky Blue
            }

            @Override
            public void mouseExited(MouseEvent e) {
                button.setBackground(backgroundColor);
            }
        });
        return button;
    }


    private JTabbedPane createTabbedPane() {
        JTabbedPane tabbedPane = new JTabbedPane() {
            @Override
            public void paintComponent(Graphics g) {
                Graphics2D g2d = (Graphics2D) g;
                g2d.setPaint(new GradientPaint(0, 0, Color.WHITE, 0, getHeight(), new Color(245, 245, 245)));
                g2d.fillRect(0, 0, getWidth(), getHeight());
                super.paintComponent(g);
            }
        };

        tabbedPane.setFont(new Font("SansSerif", Font.BOLD, 14));
        tabbedPane.setForeground(Color.DARK_GRAY);
        String[] benchmarks = {
                "All Benchmarks", "Static Memory Access", "Dynamic Memory Access",
                "Memory Allocation", "Memory Deallocation", "Thread Creation",
                "Context Switch", "Thread Migration"
        };

        for (String benchmark : benchmarks) {
            JPanel panel = createBenchmarkPanel(benchmark);
            tabbedPane.addTab(benchmark, panel);
        }

        tabbedPane.addMouseMotionListener(new MouseAdapter() {
            @Override
            public void mouseMoved(MouseEvent e) {
                for (int i = 0; i < tabbedPane.getTabCount(); i++) {
                    Rectangle bounds = tabbedPane.getBoundsAt(i);
                    if (bounds.contains(e.getPoint())) {
                        tabbedPane.setBackgroundAt(i, new Color(173, 216, 230)); // Light blue on hover
                    } else {
                        tabbedPane.setBackgroundAt(i, Color.WHITE);
                    }
                }
            }
        });

        return tabbedPane;
    }



    private void setupUI() {
        setTitle("Benchmarking Application");
        setSize(1000, 800);
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            e.printStackTrace();
        }

        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(new EmptyBorder(10, 10, 10, 10));
        mainPanel.setBackground(Color.WHITE);

        // Input Panel
        JPanel inputPanel = new JPanel(new GridBagLayout());
        inputPanel.setBorder(BorderFactory.createTitledBorder(BorderFactory.createEtchedBorder(),
                "Benchmark Settings", TitledBorder.CENTER, TitledBorder.TOP,
                new Font("SansSerif", Font.BOLD, 16), Color.DARK_GRAY));
        inputPanel.setBackground(new Color(240, 248, 255)); // Light Blue Background

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(10, 10, 10, 10);
        gbc.fill = GridBagConstraints.HORIZONTAL;

        // Number of Tests Input
        gbc.gridx = 0;
        gbc.gridy = 0;
        JLabel numTestsLabel = new JLabel("Number of Tests:");
        numTestsLabel.setFont(new Font("SansSerif", Font.PLAIN, 14));
        inputPanel.add(numTestsLabel, gbc);

        gbc.gridx = 1;
        numTestsField = new JTextField("100");
        numTestsField.setFont(new Font("SansSerif", Font.PLAIN, 14));
        numTestsField.setBorder(BorderFactory.createLineBorder(Color.GRAY));
        inputPanel.add(numTestsField, gbc);

        // Threshold Input
        gbc.gridx = 0;
        gbc.gridy = 1;
        JLabel thresholdLabel = new JLabel("Threshold:");
        thresholdLabel.setFont(new Font("SansSerif", Font.PLAIN, 14));
        inputPanel.add(thresholdLabel, gbc);

        gbc.gridx = 1;
        thresholdField = new JTextField("2");
        thresholdField.setFont(new Font("SansSerif", Font.PLAIN, 14));
        thresholdField.setBorder(BorderFactory.createLineBorder(Color.GRAY));
        inputPanel.add(thresholdField, gbc);

        // Buttons
        gbc.gridx = 0;
        gbc.gridy = 2;
        JButton resetButton = createStyledButton("Reset", new Color(255, 182, 193));
        resetButton.addActionListener(e -> resetInputs());
        inputPanel.add(resetButton, gbc);

        gbc.gridx = 1;
        JButton helpButton = createStyledButton("Help", new Color(173, 216, 230));
        helpButton.addActionListener(e -> showHelpDialog());
        inputPanel.add(helpButton, gbc);

        mainPanel.add(inputPanel, BorderLayout.NORTH);

        // Tabbed Pane
        JTabbedPane tabbedPane = createTabbedPane();
        mainPanel.add(tabbedPane, BorderLayout.CENTER);

        // Log Panel
        JPanel logPanel = new JPanel(new BorderLayout(10, 10));
        logPanel.setBackground(Color.WHITE);

        logArea = new JTextArea(15, 50);
        logArea.setEditable(false);
        logArea.setBackground(new Color(245, 245, 245));
        logArea.setFont(new Font("Monospaced", Font.PLAIN, 14));
        JScrollPane logScrollPane = new JScrollPane(logArea);
        logScrollPane.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(Color.GRAY),
                "Logs", TitledBorder.LEFT, TitledBorder.TOP,
                new Font("SansSerif", Font.BOLD, 16), Color.DARK_GRAY));
        logPanel.add(logScrollPane, BorderLayout.CENTER);

        progressBar = new JProgressBar();
        progressBar.setStringPainted(true);
        progressBar.setBackground(new Color(220, 220, 220));
        progressBar.setForeground(new Color(100, 149, 237));
        progressBar.setFont(new Font("SansSerif", Font.BOLD, 12));
        progressBar.setBorder(BorderFactory.createLineBorder(Color.GRAY));
        logPanel.add(progressBar, BorderLayout.SOUTH);

        mainPanel.add(logPanel, BorderLayout.EAST);

        add(mainPanel);
    }

    private JPanel createBenchmarkPanel(String benchmark) {
        JPanel panel = new JPanel(new BorderLayout(10, 10));
        JButton runButton = createStyledButton("Open " + benchmark, Color.LIGHT_GRAY);
        runButton.addActionListener(e -> openExplanationPage(benchmark));
        panel.add(runButton, BorderLayout.CENTER);
        return panel;
    }

    private void openExplanationPage(String benchmark) {
        // Create a new frame for the explanation
        JFrame explanationFrame = new JFrame(benchmark + " Details");
        explanationFrame.setSize(700, 500);
        explanationFrame.setLocationRelativeTo(this);
        explanationFrame.setLayout(new BorderLayout(10, 10));

        // Create a header panel with a title
        JPanel headerPanel = new JPanel(new BorderLayout());
        JLabel titleLabel = new JLabel(benchmark + " Details", JLabel.CENTER);
        titleLabel.setFont(new Font("Arial", Font.BOLD, 20));
        titleLabel.setForeground(new Color(30, 144, 255));
        headerPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        headerPanel.add(titleLabel, BorderLayout.CENTER);

        // Get HTML explanation
        String htmlContent = benchmarkExplanations.getOrDefault(benchmark,
                "<html><body style='font-family:sans-serif;'>"
                        + "<h2 style='color:darkred;'>Explanation not available</h2></body></html>");

        // Create a JEditorPane to display explanations with HTML content
        JEditorPane explanationPane = new JEditorPane();
        explanationPane.setContentType("text/html");
        explanationPane.setText(htmlContent);
        explanationPane.setEditable(false);
        explanationPane.setMargin(new Insets(10, 10, 10, 10));

        // Add a scroll pane for the explanation pane
        JScrollPane explanationScrollPane = new JScrollPane(explanationPane);
        explanationScrollPane.setBorder(BorderFactory.createTitledBorder("Detailed Explanation"));

        // Add additional notes
        JTextPane additionalNotes = new JTextPane();
        additionalNotes.setContentType("text/html");
        additionalNotes.setText(
                "<html><body style='font-family:sans-serif; color:black;'>"
                        + "<b>Notes:</b><ul>"
                        + "<li>Performance results may vary based on hardware and OS.</li>"
                        + "<li>Ensure JVM is configured for optimal performance in Java.</li>"
                        + "<li>For C/C++, use optimized compilers for accurate results.</li>"
                        + "</ul></body></html>"
        );
        additionalNotes.setEditable(false);
        additionalNotes.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        JScrollPane notesScrollPane = new JScrollPane(additionalNotes);
        notesScrollPane.setBorder(BorderFactory.createTitledBorder("Additional Notes"));

        // Create a styled Start Benchmark button
        JButton startButton = createStyledButton("Start Benchmark", new Color(50, 205, 50));
        startButton.addActionListener(e -> startBenchmark(benchmark));
        JPanel buttonPanel = new JPanel();
        buttonPanel.add(startButton);

        // Organize the main content panel
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.add(explanationScrollPane, BorderLayout.CENTER);
        mainPanel.add(notesScrollPane, BorderLayout.SOUTH);

        // Add components to the frame
        explanationFrame.add(headerPanel, BorderLayout.NORTH);
        explanationFrame.add(mainPanel, BorderLayout.CENTER);
        explanationFrame.add(buttonPanel, BorderLayout.PAGE_END);

        // Set frame visibility
        explanationFrame.setVisible(true);
    }




    private void showHelpDialog() {
        JOptionPane.showMessageDialog(
                this,
                "<html><body style='text-align:justify;'>"
                        + "<h2 style='color:darkblue;'>Help Information</h2>"
                        + "<p><b>Number of Tests:</b> Defines the iterations for benchmarks (Recommended maximum: 100).</p>"
                        + "<p><b>Threshold:</b> Removes outliers; lower values are stricter. If threshold is 2 values that are not in between mean-+2*StdDeviation are ignored. (Recommended: 2).</p>"
                        + "<p>Adjust these to balance accuracy and runtime.</p></body></html>",
                "Help",
                JOptionPane.INFORMATION_MESSAGE
        );
    }

    private Map<String, String> loadBenchmarkExplanations() {
        Map<String, String> explanations = new HashMap<>();
        explanations.put("All Benchmarks",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>All Benchmarks</h2>"
                        + "<p>This section provides an overview of various benchmarks and their performance characteristics in different programming languages.</p>"
                        + "<h3 style='color:darkgreen;'>Benchmarks</h3>"
                        + "<ul>"
                        + "<li><b>Static Memory Access:</b> Accessing static memory locations.</li>"
                        + "<li><b>Dynamic Memory Access:</b> Accessing dynamically allocated memory.</li>"
                        + "<li><b>Memory Allocation:</b> Allocating memory for variables or objects.</li>"
                        + "<li><b>Memory Deallocation:</b> Freeing allocated memory to avoid leaks.</li>"
                        + "<li><b>Thread Creation:</b> Creating, starting, and joining threads.</li>"
                        + "<li><b>Context Switch:</b> Switching between threads.</li>"
                        + "<li><b>Thread Migration:</b> Moving threads between CPU cores.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Performance varies across languages due to memory management, threading models, and runtime environments. "
                        + "Low-level languages like C and C++ offer faster memory access, while Java prioritizes safety and convenience. For a lot of iterations or large array sizes, Java becomes better.</p>"
                        + "</body></html>");
        explanations.put("Static Memory Access",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Static Memory Access</h2>"
                        + "<p>Static memory access refers to accessing variables that are allocated at compile time, "
                        + "such as global or static variables.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C and C++:</b> Static variables are stored in the data segment of memory. "
                        + "This ensures extremely fast access due to fixed memory addresses determined at compile time. Minimal overhead makes this very efficient.</li>"
                        + "<li><b>Java:</b> Static variables are stored in the method area (or metaspace). JVM adds an indirection layer and safety checks, "
                        + "leading to slight overhead. However, JIT optimizations in Java improve performance. Static arrays are still allocated on the heap in Java.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Static memory access is generally faster in <b>C/C++</b> for small array sizes due to direct access mechanisms, "
                        + "while Java incurs slight overhead from its managed runtime environment. For large array sizes(1000+), Java starts to be efficent.</p>"
                        + "</body></html>");

        explanations.put("Dynamic Memory Access",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Dynamic Memory Access</h2>"
                        + "<p>Dynamic memory access involves working with memory that is allocated at runtime.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C:</b> Uses <code>malloc</code> for allocation and <code>free</code> for deallocation. While efficient, manual management is prone to errors.</li>"
                        + "<li><b>C++:</b> Uses <code>new</code> and <code>delete</code>. It adds safety features like constructors and destructors, but slightly increases overhead. Smart pointers provide automated memory management with minor overhead.</li>"
                        + "<li><b>Java:</b> Uses <code>new</code> for allocation, with garbage collection handling deallocation. This reduces errors but adds runtime overhead during garbage collection.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Dynamic memory access tends to be faster in <b>C</b> and <b>C++</b> for low array sizes due to the absence of garbage collection. "
                        + "<b>Java</b> trades performance for safety and convenience. When large array sizes (1000+), <b>Java</b> outperforms <b>C</b> and <b>C++</b> </p>"
                        + "</body></html>");

        explanations.put("Memory Allocation",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Memory Allocation</h2>"
                        + "<p>Memory allocation is the process of reserving memory for variables or objects during runtime.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C:</b> Uses <code>malloc</code> for memory allocation. It is fast but does not initialize memory.</li>"
                        + "<li><b>C++:</b> Uses <code>new</code>, which allocates and initializes memory. Slightly slower than <code>malloc</code> but safer.</li>"
                        + "<li><b>Java:</b> Memory is allocated in the JVM heap. Optimization improves allocation speed, but garbage collection adds runtime overhead.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p><b>C</b> is the fastest for memory allocation, followed by <b>C++</b>. <b>Java</b> is slower due to JVM management. For large array sizes (1000+), <b>Java</b> outperforms <b>C</b> and <b>C++</b> </p>"
                        + "</body></html>");

        // Repeat similar structure for other benchmarks
        explanations.put("Memory Deallocation",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Memory Deallocation</h2>"
                        + "<p>Memory deallocation frees previously allocated memory to avoid memory leaks.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C:</b> Memory is freed using <code>free</code>. Efficient but error-prone if mishandled.</li>"
                        + "<li><b>C++:</b> Uses <code>delete</code>. Smart pointers automate deallocation in modern C++ with slight overhead.</li>"
                        + "<li><b>Java:</b> Garbage collection automates deallocation but adds unpredictable runtime overhead.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Manual memory management in <b>C</b> and <b>C++</b> is faster and deterministic, whereas <b>Java</b> prioritizes safety over speed. For small array sizer <b>Java's</b> execution time is very large, but it becomes better than the other languages for 1 million+ sizes.  </p>"
                        + "</body></html>");

        explanations.put("Thread Creation",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Thread Creation</h2>"
                        + "<p>Thread creation measures the time required to create, start, and join threads.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C:</b> Uses <code>pthread_create</code>. Creation is lightweight and efficient due to minimal abstraction.</li>"
                        + "<li><b>C++:</b> Uses the <code>std::thread</code> class, which adds minimal overhead for easier thread management.</li>"
                        + "<li><b>Java:</b> Threads are managed by the JVM. Additional safety and lifecycle management cause slightly higher overhead.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Thread creation in <b>C</b> and <b>C++</b> is faster due to lower abstraction layers, while <b>Java</b> prioritizes manageability and safety.</p>"
                        + "</body></html>");

        explanations.put("Context Switch",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Thread Context Switch</h2>"
                        + "<p>Thread context switching occurs when the CPU switches from one thread to another, saving and restoring thread states.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C and C++:</b> Minimal overhead due to direct OS calls for context switching. Performance depends on OS efficiency.</li>"
                        + "<li><b>Java:</b> Context switching is managed by the JVM and operating system. JVM adds slight overhead to manage thread safety and scheduling.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Low-level languages like <b>C</b> and <b>C++</b> offer faster context switching for all iterations, while <b>Java</b> incurs additional JVM overhead at less iterations but as iterations increase it catches up with the other languages.</p>"
                        + "</body></html>");

        explanations.put("Thread Migration",
                "<html><body style='font-family:sans-serif; padding:10px;'>"
                        + "<h2 style='color:darkblue;'>Thread Migration</h2>"
                        + "<p>Thread migration measures the time required for a thread to move from one CPU core to another.</p>"
                        + "<h3 style='color:darkgreen;'>Comparison</h3>"
                        + "<ul>"
                        + "<li><b>C and C++:</b> Uses OS-level APIs for controlling thread affinity. Migration overhead is dependent on OS scheduler efficiency.</li>"
                        + "<li><b>Java:</b> Thread migration is abstracted by the JVM. Although efficient, JVM adds a layer of indirection, slightly increasing latency.</li>"
                        + "</ul>"
                        + "<h3 style='color:darkred;'>Key Takeaway</h3>"
                        + "<p>Thread migration is almost equally fast in all 3 languages, but it is much slower as iterations increase.</p>"
                        + "</body></html>");

        return explanations;
    }


    private void startBenchmark(String benchmark) {
        if (isBenchmarkInProgress) {
            logArea.append("Benchmarking is already in progress. Please wait for the current benchmark to complete.\n");
            return;
        }
        SwingWorker<Void, String> worker = new SwingWorker<>() {
            @Override
            protected Void doInBackground() {
                try {
                    isBenchmarkInProgress = true;
                    int numTests = Integer.parseInt(numTestsField.getText());
                    double threshold = Double.parseDouble(thresholdField.getText());

                    publish("Running warm-up...");
                    controller.runBenchmark("C", 0, numTests, threshold);
                    publish("Warm-up completed.");

                    publish("Running " + benchmark + "...");
                    runBenchmarkForAllLanguages(benchmark, numTests, threshold);
                    //publish(benchmark + " completed.");
                } catch (Exception ex) {
                    publish("Error: " + ex.getMessage());
                }
                return null;
            }

            @Override
            protected void process(java.util.List<String> chunks) {
                for (String message : chunks) {
                    logArea.append(message + "\n");
                    logArea.setCaretPosition(logArea.getDocument().getLength());
                }
            }

//            @Override
//            protected void done() {
//                progressBar.setValue(100);
//            }
        };

        worker.execute();
    }

    private void runBenchmarkForAllLanguages(String benchmark, int numTests, double threshold) {
        SwingWorker<Void, Integer> worker = new SwingWorker<>() {
            @Override
            protected Void doInBackground() {
                String[] languages = {"C++", "C", "Java"};
                int totalSteps = languages.length;
                int step = 0;

                for (String language : languages) {
                    try {
                        controller.runBenchmark(language, getBenchmarkType(benchmark), numTests, threshold);
                        controller.loadResults(language, getBenchmarkType(benchmark));
                        step++;
                        int progress = (int) ((step / (double) totalSteps) * 100);
                        publish(progress);
                    } catch (Exception ex) {
                        logArea.append("Error: " + ex.getMessage() + "\n");
                    }
                }

                if (benchmark.equals("All Benchmarks")) {
                    for (String allBenchmarks : new String[]{
                            "Static Memory Access", "Dynamic Memory Access", "Memory Allocation",
                            "Memory Deallocation", "Thread Creation", "Context Switch", "Thread Migration"
                    }) {
                        controller.generateGraph(allBenchmarks);
                    }
                } else {
                    controller.generateGraph(benchmark);
                }
                return null;
            }

            @Override
            protected void process(java.util.List<Integer> chunks) {
                for (int progress : chunks) {
                    progressBar.setValue(progress);
                    progressBar.setForeground(progress < 50 ? Color.YELLOW : Color.GREEN);
                }
            }

            @Override
            protected void done() {
                progressBar.setValue(100);
                logArea.append("Benchmarking Completed.\n");
                isBenchmarkInProgress = false;
            }
        };

        worker.execute();
    }


    private void resetInputs() {
        if (!isBenchmarkInProgress) {
            numTestsField.setText("100");
            thresholdField.setText("2");
            logArea.setText("");
            progressBar.setValue(0);
        }
    }

    private int getBenchmarkType(String benchmarkName) {
        switch (benchmarkName) {
            case "All Benchmarks":
                return 0;
            case "Static Memory Access":
                return 1;
            case "Dynamic Memory Access":
                return 2;
            case "Memory Allocation":
                return 3;
            case "Memory Deallocation":
                return 4;
            case "Thread Creation":
                return 5;
            case "Context Switch":
                return 6;
            case "Thread Migration":
                return 7;
            default:
                return -1;
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new MainGUI().setVisible(true));
//        Controller controller = new Controller();
//        controller.loadResults("C", 0);
//        controller.loadResults("C++", 0);
//        controller.loadResults("Java", 0);
//        controller.generateGraph("Static Memory Access");
//        controller.generateGraph("Dynamic Memory Access");
//        controller.generateGraph("Memory Allocation");
//        controller.generateGraph("Memory Deallocation");
//        controller.generateGraph("Thread Creation");
//        controller.generateGraph("Context Switch");
//        controller.generateGraph("Thread Migration");
    }
}
