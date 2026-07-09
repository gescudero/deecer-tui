CC = gcc
CFLAGS = -Wall -g -fsanitize=address 
LDFLAGS = -lncursesw -ltinfow -lcurl -lcjson -lmpv -lpthread -static-libasan
TARGET = deecer-tui

SRC_DIR = src
BUILD_DIR = build

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

