# Makefile for uWebSockets minimal project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -IuWebSockets -IuSockets/src -Ideps -flto -Iinclude

# Source files
SRC = main.cpp

# Output binary
OUT = server

# Libraries
LIBS = -lpthread -lssl -lcrypto -lz

# Targets
all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)

.PHONY: all clean
