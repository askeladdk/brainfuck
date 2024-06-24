CC     := gcc
CFLAGS := -Wall
SRC    := $(wildcard *.c)
HDR    := $(wildcard *.h)
EXE    := brainfuck

all: $(EXE)

$(EXE): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -O3 $(SRC) -o $(EXE)

$(EXE).debug: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -g $(SRC) -o $(EXE).debug

clean:
	rm -f $(EXE) $(EXE).debug
