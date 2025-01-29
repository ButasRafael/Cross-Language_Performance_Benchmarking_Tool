# README.txt

## Overview
This project uses a `Makefile` to simplify the process of compiling and running a Java application that integrates with native code through JNI (Java Native Interface). The `Makefile` automates the compilation of Java files, the generation of JNI headers, and the building of shared native libraries. This makes development and execution more streamlined and reduces manual errors.

## What the Makefile Does

1. **Java Compilation**: Compiles all `.java` source files and generates the corresponding `.class` files and JNI headers.
2. **Native Libraries**: Compiles C, C++, and other native source files into shared libraries (`.so` files) required for the application.
3. **Execution**: Runs the main Java application with the necessary library and classpath settings.
4. **Cleanup**: Removes all generated files (class files, headers, and shared libraries) to ensure a fresh build.

## Why the Makefile is Helpful
- **Automation**: Automates repetitive tasks like compilation, header generation, and library linking.
- **Consistency**: Ensures all steps are performed with the correct flags and dependencies every time.
- **Ease of Use**: Simplifies the process of running the application with a single command.
- **Cross-Language Integration**: Manages the complexity of combining Java with C/C++ code through JNI.

## How to Use the Makefile

1. **Build the Project**:
   To compile the Java source files and native libraries, run:
   ```
   make
   ```

2. **Run the Application**:
   After building the project, execute the application with:
   ```
   make run
   ```

3. **Clean the Build Artifacts**:
   To remove all generated files and start fresh, use:
   ```
   make clean
   ```

## Important Variables in the Makefile
- **`JAVA_HOME`**: Specifies the path to your JDK installation.
- **`JAR_DEPENDENCIES`**: Defines the external libraries (JAR files) required by the project.
- **`CFLAGS` and `LDFLAGS`**: Specify compilation and linking options for native code.

## Notes
- Ensure that your `JAVA_HOME` path matches your system's JDK installation.
- The shared libraries (`.so` files) are automatically linked and made available for the Java application during runtime.

This `Makefile` provides a robust and efficient workflow for developing and running your Java-JNI integrated application. Happy coding!

