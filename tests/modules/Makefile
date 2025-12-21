# Makefile for AK24 Module Integration Test
#
# This Makefile builds the integration test against the INSTALLED ak24 library.
# It does NOT use the local source code - only the installed headers and libraries.
#
# Environment:
#   AK24_HOME - Installation directory (default: ~/.ak24)
#

# Installation directory
AK24_HOME ?= $(HOME)/.ak24

# Check if AK24 is installed
ifeq (,$(wildcard $(AK24_HOME)/include/ak24/kernel.h))
    $(error AK24 not found at $(AK24_HOME). Please install AK24 first with 'make install')
endif

# Load AK24 build configuration if available
-include $(AK24_HOME)/lib/ak24-config.mk

# Compiler and flags
CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2
INCLUDES = -I$(AK24_HOME)/include
LDFLAGS = -L$(AK24_HOME)/lib
LIBS = -lak24_kernel

# Add ASAN flags if AK24 was built with ASAN
ifeq ($(AK24_ASAN_ENABLED),1)
    CFLAGS += $(AK24_ASAN_FLAGS)
    LDFLAGS += $(AK24_ASAN_FLAGS)
    $(info Building with AddressSanitizer support)
else
    LIBS += -lgc
endif

# macOS-specific: Add rpath for finding dylib
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -Wl,-rpath,$(AK24_HOME)/lib -Wl,-w
endif

# Linux-specific: Add rpath for finding .so
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -Wl,-rpath=$(AK24_HOME)/lib
endif

# Build target
TARGET = module_test

all: $(TARGET)

$(TARGET): main.c
	@echo "=== Building Module Integration Test ==="
	@echo "Using AK24 from: $(AK24_HOME)"
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $< $(LIBS)
	@echo "✓ Build complete: $(TARGET)"
	@echo ""
	@echo "Run with: ./$(TARGET)"

clean:
	rm -f $(TARGET)
	rm -rf build

distclean: clean
	@echo "✓ Clean complete"

.PHONY: all clean distclean
