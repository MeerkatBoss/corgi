/**
 * @file Error.c
 * @author Ivan Solodovnikov (solodovnikov.ia@phystech.edu)
 * @brief File operation error code utilities
 * @version 0.1
 * @date 2025-11-29
 *
 * @copyright Ivan Solodovnikov (c) 2025
 */
#include "Files/Error.h"

const char* file_error_to_string(file_error_t error) {
  switch (error) {
  case FERR_NONE:
    return "No error";
  case FERR_INVALID_VALUE:
    return "Invalid value";
  case FERR_INVALID_OPERATION:
    return "Invalid operation";
  case FERR_ACCESS_DENIED:
    return "Access denied";
  case FERR_ALREADY_EXISTS:
    return "File already exists";
  default:
    return "Unknown error";
  }
}
