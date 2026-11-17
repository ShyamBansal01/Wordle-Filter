CXX         ?= g++
CXXFLAGS    := -std=c++14 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude

BUILD_DIR   := build
LIB_SRC     := src/wordle_filter.cpp
APP_SRC     := src/main.cpp
TEST_SRC    := tests/test_wordle_filter.cpp
HEADERS     := include/wordle_filter.hpp

ifeq ($(OS),Windows_NT)
    SHELL       := cmd.exe
    APP_TARGET  := $(BUILD_DIR)\wordle_solver.exe
    TEST_TARGET := $(BUILD_DIR)\run_tests.exe
    MKDIR       = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM          = if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
    RUN_PREFIX  =
else
    APP_TARGET  := $(BUILD_DIR)/wordle_solver
    TEST_TARGET := $(BUILD_DIR)/run_tests
    MKDIR       = mkdir -p $(BUILD_DIR)
    RM          = rm -rf $(BUILD_DIR)
    RUN_PREFIX  = ./
endif

.PHONY: all run test clean

all: $(APP_TARGET) $(TEST_TARGET)

$(APP_TARGET): $(LIB_SRC) $(APP_SRC) $(HEADERS)
	$(MKDIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(APP_SRC) -o $(APP_TARGET)

$(TEST_TARGET): $(LIB_SRC) $(TEST_SRC) $(HEADERS)
	$(MKDIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(TEST_SRC) -o $(TEST_TARGET)

run: $(APP_TARGET)
	$(RUN_PREFIX)$(APP_TARGET) data/WORDS

test: $(TEST_TARGET)
	$(RUN_PREFIX)$(TEST_TARGET)

clean:
	$(RM)