CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = 
TARGET = myprog

SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

debug: CFLAGS += -g -DDEBUG
debug: rebuild

release: CFLAGS += -O2
release: rebuild

.PHONY: all clean rebuild install debug release