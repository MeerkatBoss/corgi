/**
 * @file Assert.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.su)
 *
 * @brief Macros for handling unrecoverable errors
 *
 * @version 0.1
 * @date 2026-02-07
 *
 * @copyright Copyright MeerkatBoss (c) 2026
 */
#ifndef __COMMON_PANIC_H
#define __COMMON_PANIC_H

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Abort with message for unrecoverable errors
 *
 * Unlike assert(), always active regardless of NDEBUG.
 */
#define PANIC(msg)                                      \
  do {                                                  \
    fprintf(stderr, "PANIC: %s\n  at %s:%d in %s()\n",  \
            (msg), __FILE__, __LINE__, __func__);       \
    abort();                                            \
  } while (0)

/**
 * @brief Abort if memory allocation failed
 */
#define PANIC_ON_BAD_ALLOC(ptr)           \
  do {                                    \
    if ((ptr) == NULL) {                  \
      PANIC("memory allocation failed");  \
    }                                     \
  } while (0)

/**
 * @brief Abort if argument is NULL (programmer error)
 */
#define PANIC_IF_NULL(ptr)                      \
  do {                                          \
    if ((ptr) == NULL) {                        \
      PANIC("unexpected NULL argument: " #ptr); \
    }                                           \
  } while (0)

#endif /* Panic.h */
