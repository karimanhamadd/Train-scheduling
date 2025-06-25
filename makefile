#from tutorial 4 
CC = gcc
CFLAGS = -Wall -pthread
TARGETS = mts

# Define the default target
all: $(TARGETS)

# Compile mts.c as your program
mts: mts.c
	$(CC) $(CFLAGS) -o mts mts.c

# Clean up compiled files
clean:
	rm -f $(TARGETS)

.PHONY: all clean
