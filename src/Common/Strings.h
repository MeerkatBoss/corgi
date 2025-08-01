/**
 * @file Strings.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief String utility functions
 *
 * @version 0.1
 * @date 2025-08-01
 *
 * @copyright Copyright MeerkatBoss (c) 2025
 */
#ifndef __COMMON_STRINGS_H
#define __COMMON_STRINGS_H

#include <stddef.h>

/**
 * Creates a copy of the given string
 * @param str The string to copy
 * @return A newly allocated copy of the string, or NULL if str is NULL or allocation fails
 */
char* copy_string(const char* str);

/**
 * Appends a string to a buffer
 * @param buf The destination buffer
 * @param buf_size The size of the buffer
 * @param str The string to append
 * @return The total length that would be needed (including what was already in buf)
 */
unsigned long append_string(char* buf, unsigned long buf_size, const char* str);

#endif /* Strings.h */ 