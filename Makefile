CC = gcc

# flags comunes
BASE_CFLAGS = -Wall -g 
BASE_LDFLAGS = -lncursesw -ltinfow -lcurl -lcjson -lmpv -lpthread \
		  -lssl -lcrypto -L./lib -Wl,-Bstatic -ldeezer_crypto -Wl,-Bdynamic

# Flags para release
CFLAGS = $(BASE_CFLAGS)
LDFLAGS = $(BASE_LDFLAGS)
TARGET = deecer-tui

SRC_DIR = src
BUILD_DIR = build

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# release
all: $(TARGET)

# debug
debug: CFLAGS = $(BASE_CFLAGS) -fsanitize=address
debug: LDFLAGS = $(BASE_LDFLAGS) -static-libasan
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all debug clean run

