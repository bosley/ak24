.PHONY: all clean configure build test install

BUILD_DIR := build
BUILD_TYPE ?= Release
AK24_GC ?= ON
JOBS := $(shell sysctl -n hw.ncpu 2>/dev/null || echo 4)

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DAK24_GC_ENABLED=$(AK24_GC) ..

build: configure
	@cmake --build $(BUILD_DIR) -j$(JOBS)

test: build
	@$(BUILD_DIR)/bin/ak24_tests

clean:
	@rm -rf $(BUILD_DIR)

install: build
	@cmake --install $(BUILD_DIR)

