CC = gcc
CFLAGS = -Iinclude -lm -Wall
SRC = src/main.c src/file_parser.c
OUTPUT = kmeans_program.exe

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)