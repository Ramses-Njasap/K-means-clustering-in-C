CC = gcc
CFLAGS = -Iinclude -Wall
LDFLAGS = -lSDL2 -lSDL2_ttf -lm
SRC = src/main.c src/file_parser.c
OUTPUT = kmeans_program

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)