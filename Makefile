CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
GTK_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK_LIBS := $(shell pkg-config --libs gtk4)

TARGET := calculator
SRC := main.c src/ui.c src/calc_eval.c src/style_manager.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -Iinclude $(GTK_CFLAGS) -o $@ $^ $(GTK_LIBS) -lm

clean:
	rm -f $(TARGET)
