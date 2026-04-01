COUNT=10

SRC = config.c test.c parser.c

LIB_FLAGS = -lgraphblas -llagraph -llagraphx
INCLUDE_FLAGS = -I/usr/local/include/suitesparse -I./

build: config.c test.c testAP.c testSP.c parser.c
	gcc test.c parser.c config.c -O2 -L/usr/local/lib -lgraphblas -llagraph -llagraphx -Wl,-rpath,/usr/local/lib -I/usr/local/include/suitesparse -I./ -o test && \
	gcc testSP.c parser.c config.c -O2 -L/usr/local/lib -lgraphblas -llagraph -llagraphx -Wl,-rpath,/usr/local/lib -I/usr/local/include/suitesparse -I./ -o testSP && \
	gcc testAP.c parser.c config.c -O2 -L/usr/local/lib -lgraphblas -llagraph -llagraphx -Wl,-rpath,/usr/local/lib -I/usr/local/include/suitesparse -I./ -o testAP\

all:
	for algo in test testSP testAP; do \
		./$$algo; \
	done \

build-debug: $(SRC)
	gcc $(SRC) ${ALGO} ${LIB_FLAGS} ${INCLUDE_FLAGS} \
 -o test -g -Wextra -Wno-sign-compare -pedantic -fsanitize=undefined \
 

run: $(SRC)
	gcc $(SRC) ${ALGO} ${LIB_FLAGS} ${INCLUDE_FLAGS} \
 -o test -Wextra -Wno-sign-compare -pedantic -fsanitize=undefined -DDEBUG_parser \
 && ./test

bench: $(SRC)
	gcc $(SRC) -O2 -L/usr/local/lib -lgraphblas -llagraph -llagraphx -Wl,-rpath,/usr/local/lib -I/usr/local/include/suitesparse -I./ -o test && ./test

bench-CI: $(SRC)
	gcc $(SRC) -O2 -DCI ${ALGO} ${LIB_FLAGS} ${INCLUDE_FLAGS} \
		-o test && ./test

debug: $(SRC)
	gcc $(SRC) ${ALGO} \
${LIB_FLAGS} ${INCLUDE_FLAGS} \
 -g -o test -Wextra -Wall -pedantic -fsanitize=address,undefined -DDEBUG \
 && ./test

debug-info: $(SRC)
	gcc $(SRC) ${ALGO} \
${LIB_FLAGS} ${INCLUDE_FLAGS} \
 -g -o test -Wno-sign-compare -pedantic -DDEBUG \
 && ./test

convert: convert.c
	gcc convert.c -o convert -Wextra -Wall -pedantic && time ./convert

benchAP: testAP.c parser.c config.c
	gcc testAP.c parser.c config.c -O2 -L/usr/local/lib -lgraphblas -llagraph -llagraphx -Wl,-rpath,/usr/local/lib -I/usr/local/include/suitesparse -I./ -o testAP && ./testAP
	
benchSP: testSP.c parser.c config.c
	gcc testSP.c parser.c config.c -O2 -L/usr/local/lib -lgraphblas -llagraph -llagraphx -Wl,-rpath,/usr/local/lib -I/usr/local/include/suitesparse -I./ -o testSP && ./testSP
