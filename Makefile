# weathercli - Terminal weather forecast application
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -I vendor
LDFLAGS = -lcurl -lncursesw

SRC_DIR   = src
VENDOR_DIR = vendor
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/http.c \
          $(SRC_DIR)/location.c \
          $(SRC_DIR)/weather.c \
          $(SRC_DIR)/gpt.c \
          $(SRC_DIR)/secrets.c \
          $(SRC_DIR)/activity.c \
          $(SRC_DIR)/ui.c \
          $(VENDOR_DIR)/cJSON.c

OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SOURCES)))
TARGET  = $(BUILD_DIR)/weathercli

.PHONY: all clean run

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

HEADERS = $(wildcard $(SRC_DIR)/*.h)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cJSON.o: $(VENDOR_DIR)/cJSON.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
