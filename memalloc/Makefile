CXX = g++
CXXFLAGS = -g -Wall

SRCS = test.cpp alloc.cpp
INCLUDES = -I.

TARGET = test.o

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

debug: $(TARGET)
	gdb ./$(TARGET)

.PHONY: all debug