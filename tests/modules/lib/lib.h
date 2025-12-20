/**
 * @file lib.h
 * @brief Test module interface for AK24 module system integration testing
 *
 * This module implements the standard AK24 module ABI:
 * - ak_module_version() - Returns API version
 * - ak_module_init() - Receives allocator interface
 * - ak_module_deinit() - Cleanup
 * - ak_module_info() - Module metadata
 * - ak_module_get_function() - Optional function dispatch
 */

#ifndef TEST_MODULE_LIB_H
#define TEST_MODULE_LIB_H

// Module uses standard AK24 ABI - no custom header needed
// Module exports will be loaded via dlsym

#endif
