CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -municode -static -mwindows
TARGET = bin/main.exe

all: $(TARGET)

.PHONY: all clean install test

$(TARGET): main.c
	mkdir -p bin && \
	$(CC) $(CFLAGS) -o $@ $<

clean: 
		rm -f $(TARGET) 