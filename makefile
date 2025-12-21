.PHONY: all clean configure build test install uninstall docs docs-clean

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
	@echo "Running all tests..."
	@for test in $(BUILD_DIR)/test/compile_time/ak24_*; do \
		if [ -f "$$test" ]; then \
			echo ""; \
			echo "=== Running $$(basename $$test) ==="; \
			"$$test" || exit 1; \
		fi; \
	done
	@echo ""
	@echo "==================================="
	@echo "All tests passed!"
	@echo "==================================="

docs:
	@echo "Generating API documentation with Doxygen..."
	@doxygen Doxyfile
	@echo "Documentation generated in docs/api/html/"
	@echo "Open docs/api/html/index.html to view"

docs-clean:
	@rm -rf docs/api

clean: docs-clean
	@rm -rf $(BUILD_DIR)

install: build
	@cmake --install $(BUILD_DIR)

uninstall:
	@cd $(BUILD_DIR) && cmake --build . --target uninstall

