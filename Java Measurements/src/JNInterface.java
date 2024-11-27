public class JNInterface {

    static {
        System.loadLibrary("mynative_c");
        System.loadLibrary("mynative_cpp");
    }

    public native void callNative_C_Benchmark(int benchmarkType, int numTests, double threshold);

    public native void callNative_Cpp_Benchmark(int benchmarkType, int numTests, double threshold);
}
