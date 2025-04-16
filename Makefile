CC = gcc
CFLAGS = -Iinclude -lm -Wall
SRC = src/main.c src/hello.c
OUTPUT = kmeans_program

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)