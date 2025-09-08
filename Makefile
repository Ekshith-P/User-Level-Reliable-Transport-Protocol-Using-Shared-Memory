# Makefile for C++ version

# Compiler and flags
CXX = g++
# Use C++11 standard for thread support, and -pthread for linking
CXXFLAGS = -Wall -g -std=c++11 -pthread
LDFLAGS = -L. -lksocket

# Default target: build the library and all executables
all: libksocket.a initksocket user1 user2

# Rule to create the static library
libksocket.a: ksocket.o
	ar rcs libksocket.a ksocket.o
	@echo "Library libksocket.a created"

# Rule to compile ksocket.cpp
ksocket.o: ksocket.cpp ksocket.hpp
	$(CXX) $(CXXFLAGS) -c ksocket.cpp -o ksocket.o

# Rule to create initksocket executable
initksocket: initksocket.o libksocket.a
	$(CXX) initksocket.o -o initksocket $(LDFLAGS)
	@echo "Executable initksocket created"

initksocket.o: initksocket.cpp ksocket.hpp
	$(CXX) $(CXXFLAGS) -c initksocket.cpp -o initksocket.o

# Rule to create user1 executable
user1: user1.o libksocket.a
	$(CXX) user1.o -o user1 $(LDFLAGS)
	@echo "Executable user1 created"

user1.o: user1.cpp ksocket.hpp
	$(CXX) $(CXXFLAGS) -c user1.cpp -o user1.o

# Rule to create user2 executable
user2: user2.o libksocket.a
	$(CXX) user2.o -o user2 $(LDFLAGS)
	@echo "Executable user2 created"

user2.o: user2.cpp ksocket.hpp
	$(CXX) $(CXXFLAGS) -c user2.cpp -o user2.o

# Clean up build artifacts
clean:
	rm -f *.o libksocket.a initksocket user1 user2
	@echo "Cleaned up all build files"

.PHONY: all clean