# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall

# Linker flags (if needed)
LDFLAGS = -lssl -lcrypto

# Object files
OBJS = helper.o node.o simulator.o

# Output executable
TARGET = my_program

# Default target
all: $(TARGET)

# Linking the object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compiling the .cpp files into .o (object files)
helper.o: helper.cc helper.h
	$(CXX) $(CXXFLAGS) -c helper.cc $(LDFLAGS)

node.o: node.cc node.h
	$(CXX) $(CXXFLAGS) -c node.cc

simulator.o: simulator.cc simulator.h
	$(CXX) $(CXXFLAGS) -c simulator.cc

# Clean up object files and executable
clean:
	rm  $(OBJS) $(TARGET).exe