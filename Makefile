# Lexical Analyzer Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = lexer
SRC = lexer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run-python: $(TARGET)
	./$(TARGET) test.py

run-typescript: $(TARGET)
	./$(TARGET) test.ts

.PHONY: all clean run-python run-typescript

