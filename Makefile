# Simple Makefile for SQLlite-Database

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

SOURCES = main.cpp table.cpp field.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = lite

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
