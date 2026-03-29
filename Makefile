CC = gcc

LIB_FLAGS = -lgraphblas -llagraph -llagraphx
INCLUDE_FLAGS = -I/usr/local/include/suitesparse -I./
LDFLAGS = -L/usr/local/lib -Wl,-rpath,/usr/local/lib
CFLAGS_COMMON = -Wextra -Wno-sign-compare -pedantic

SRC = test.c parser.c

build: $(SRC)
	$(CC) $(SRC) $(LIB_FLAGS) $(INCLUDE_FLAGS) -o test -O2 $(CFLAGS_COMMON)

build-debug: $(SRC)
	$(CC) $(SRC) $(LIB_FLAGS) $(INCLUDE_FLAGS) -o test -g $(CFLAGS_COMMON) -fsanitize=undefined

run: $(SRC)
	$(CC) $(SRC) $(LIB_FLAGS) $(INCLUDE_FLAGS) -o test -g $(CFLAGS_COMMON) -fsanitize=undefined -DDEBUG_parser && ./test

bench: $(SRC)
	$(CC) $(SRC) $(LDFLAGS) $(LIB_FLAGS) $(INCLUDE_FLAGS) -o test -O2 && ./test

bench-CI: $(SRC)
	$(CC) $(SRC) $(LDFLAGS) $(LIB_FLAGS) $(INCLUDE_FLAGS) -o test -O2 -DCI && ./test

debug: $(SRC)
	$(CC) $(SRC) $(LDFLAGS) $(LIB_FLAGS) $(INCLUDE_FLAGS) -g -o test $(CFLAGS_COMMON) -fsanitize=address,undefined -DDEBUG && ./test

debug-info: $(SRC)
	$(CC) $(SRC) $(LDFLAGS) $(LIB_FLAGS) $(INCLUDE_FLAGS) -g -o test -Wno-sign-compare -pedantic -DDEBUG && ./test

convert: convert.c
	$(CC) convert.c -o convert -Wextra -Wall -pedantic && time ./convert

bench_all: $(SRC)
	$(CC) $(SRC) $(LDFLAGS) $(LIB_FLAGS) $(INCLUDE_FLAGS) -o test -O2 && \
	for algo in reachability singlepath allpaths; do \
		BENCH_ALGO=$$algo ./test; \
	done \
