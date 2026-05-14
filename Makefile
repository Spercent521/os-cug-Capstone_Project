CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=
PTHREAD := -pthread

SRC_DIR := src
BIN_DIR := bin

TARGETS := \
	$(BIN_DIR)/exp1_job_scheduling \
	$(BIN_DIR)/exp2_disk_scheduling \
	$(BIN_DIR)/exp3_filetools \
	$(BIN_DIR)/exp4_process_management \
	$(BIN_DIR)/exp5_ipc \
	$(BIN_DIR)/exp6_mlfq \
	$(BIN_DIR)/exp7_readers_writers

.PHONY: all clean

all: $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/exp1_job_scheduling: $(SRC_DIR)/exp1_job_scheduling.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/exp2_disk_scheduling: $(SRC_DIR)/exp2_disk_scheduling.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/exp3_filetools: $(SRC_DIR)/exp3_filetools.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/exp4_process_management: $(SRC_DIR)/exp4_process_management.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/exp5_ipc: $(SRC_DIR)/exp5_ipc.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/exp6_mlfq: $(SRC_DIR)/exp6_mlfq.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/exp7_readers_writers: $(SRC_DIR)/exp7_readers_writers.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(PTHREAD) $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)
