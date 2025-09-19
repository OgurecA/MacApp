APP_NAME = UserDoc
BIN_NAME = userdoc

SRC_DIR  = src
BUILD_DIR= build

SRC = $(SRC_DIR)/main.c

CFLAGS  = -O2 -Wall -Wextra
LDFLAGS =

# Пакеты: GTK3 и json-glib. На мак-раннере поставим их через Homebrew.
PKG = gtk+-3.0 json-glib-1.0

CFLAGS  += $(shell pkg-config --cflags $(PKG))
LDFLAGS += $(shell pkg-config --libs   $(PKG))

all: $(BUILD_DIR)/$(BIN_NAME)

$(BUILD_DIR)/$(BIN_NAME): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
