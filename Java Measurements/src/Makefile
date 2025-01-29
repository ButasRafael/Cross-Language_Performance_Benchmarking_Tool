# Java and JNI configuration
JAVA_HOME = /usr/lib/jvm/java-11-openjdk-amd64
CFLAGS = -fPIC -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
LDFLAGS = -shared
LIBRARY_PATH = .

# Dependencies
JAR_DEPENDENCIES = json-20240303.jar:jfreechart-1.5.3.jar
CLASSPATH = .:$(JAR_DEPENDENCIES)

# Source files
JAVA_SRC = $(wildcard *.java)
JNI_HEADERS = $(wildcard *.h)
C_SRC = C_native_code.c
CPP_SRC = C++_native_code.cpp
MIGRATION_NATIVE_SRC = Thread_Migration.c

# Output files
CLASS_FILES = $(JAVA_SRC:.java=.class)
LIB_C = libmynative_c.so
LIB_CPP = libmynative_cpp.so
LIB_MIGRATION= libmynative_java.so
MAIN_CLASS = MainGUI

.PHONY: all clean run

# Default target
all: compile_java compile_native

# Compile Java files and generate headers
compile_java: $(CLASS_FILES)

%.class: %.java
	javac -cp $(CLASSPATH) -h . $<

# Compile native libraries
compile_native: $(LIB_C) $(LIB_CPP) $(LIB_MIGRATION)

$(LIB_C): $(C_SRC)
	gcc $(LDFLAGS) -o $@ $< $(CFLAGS) -lcjson

$(LIB_CPP): $(CPP_SRC)
	g++ $(LDFLAGS) -o $@ $< $(CFLAGS)

$(LIB_MIGRATION): $(MIGRATION_NATIVE_SRC)
	gcc $(LDFLAGS) -o $@ $< $(CFLAGS)

# Run the Java application
run: all
	java -Xss512m -Djava.library.path=$(LIBRARY_PATH) -cp $(CLASSPATH) $(MAIN_CLASS)

# Clean build artifacts
clean:
	rm -f *.class *.h $(LIB_C) $(LIB_CPP) $(LIB_MIGRATION)
