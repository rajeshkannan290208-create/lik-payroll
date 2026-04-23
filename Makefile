CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
WEB_TARGET = payroll_api_server
CONSOLE_TARGET = payroll_system

WEB_SOURCES = web_server.cpp ApiServer.cpp Employee.cpp Payroll.cpp
CONSOLE_SOURCES = main.cpp Employee.cpp Payroll.cpp PayrollSystem.cpp

WEB_OBJECTS = $(WEB_SOURCES:.cpp=.o)
CONSOLE_OBJECTS = $(CONSOLE_SOURCES:.cpp=.o)

ifeq ($(OS),Windows_NT)
WEB_LIBS = -lws2_32
else
WEB_LIBS = 
endif

all: $(WEB_TARGET)

$(WEB_TARGET): $(WEB_OBJECTS)
	$(CXX) $(WEB_OBJECTS) -o $(WEB_TARGET) $(WEB_LIBS)
	@echo "C++ API server built. Run with: ./$(WEB_TARGET)"

console: $(CONSOLE_TARGET)

$(CONSOLE_TARGET): $(CONSOLE_OBJECTS)
	$(CXX) $(CONSOLE_OBJECTS) -o $(CONSOLE_TARGET)
	@echo "Console app built. Run with: ./$(CONSOLE_TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(WEB_TARGET) $(CONSOLE_TARGET) $(WEB_TARGET).exe $(CONSOLE_TARGET).exe
	@echo "Cleaned up build files"

rebuild: clean all

run: $(WEB_TARGET)
	./$(WEB_TARGET)

run-console: $(CONSOLE_TARGET)
	./$(CONSOLE_TARGET)

help:
	@echo "Available targets:"
	@echo "  make             - Build the C++ API web server"
	@echo "  make run         - Build and run the C++ API web server"
	@echo "  make console     - Build the original console app"
	@echo "  make run-console - Build and run the original console app"
	@echo "  make clean       - Remove build files"

.PHONY: all console clean rebuild run run-console help
