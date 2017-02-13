# This makefile was automatically generated. Run ./generate_make_file to regenerate the file.
CC=g++
AR=ar
CFLAGS=-Wall -O3 -DNDEBUG -march=native -std=c++1y -fPIC -Iinclude
LDFLAGS=-lroutingkit

all: bin/compute_freeflow_weight bin/run_td_s bin/run_td_s_p bin/compute_time_window_weight

build/compute_freeflow_weight.o: src/compute_freeflow_weight.cpp src/ipp.h src/verify.h generate_make_file
	mkdir -p build
	$(CC) $(CFLAGS)  -c src/compute_freeflow_weight.cpp -o build/compute_freeflow_weight.o

build/run_td_s.o: src/dijkstra.h src/id_queue.h src/ipp.h src/run_td_s.cpp src/timestamp_flag.h src/verify.h generate_make_file
	mkdir -p build
	$(CC) $(CFLAGS)  -c src/run_td_s.cpp -o build/run_td_s.o

build/run_td_s_p.o: src/dijkstra.h src/id_queue.h src/ipp.h src/run_td_s_p.cpp src/timestamp_flag.h src/verify.h generate_make_file
	mkdir -p build
	$(CC) $(CFLAGS)  -c src/run_td_s_p.cpp -o build/run_td_s_p.o

build/compute_time_window_weight.o: src/compute_time_window_weight.cpp src/ipp.h src/verify.h generate_make_file
	mkdir -p build
	$(CC) $(CFLAGS)  -c src/compute_time_window_weight.cpp -o build/compute_time_window_weight.o

build/verify.o: src/verify.cpp src/verify.h generate_make_file
	mkdir -p build
	$(CC) $(CFLAGS)  -c src/verify.cpp -o build/verify.o

bin/compute_freeflow_weight: build/compute_freeflow_weight.o build/verify.o
	mkdir -p bin
	$(CC) build/compute_freeflow_weight.o build/verify.o  -o bin/compute_freeflow_weight $(LDFLAGS)

bin/run_td_s: build/run_td_s.o build/verify.o
	mkdir -p bin
	$(CC) build/run_td_s.o build/verify.o  -o bin/run_td_s $(LDFLAGS)

bin/run_td_s_p: build/run_td_s_p.o build/verify.o
	mkdir -p bin
	$(CC) build/run_td_s_p.o build/verify.o  -o bin/run_td_s_p $(LDFLAGS)

bin/compute_time_window_weight: build/compute_time_window_weight.o build/verify.o
	mkdir -p bin
	$(CC) build/compute_time_window_weight.o build/verify.o  -o bin/compute_time_window_weight $(LDFLAGS)

