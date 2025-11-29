/**
 * @file Error.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.su)
 *
 * @brief File operation error codes
 *
 * @version 0.1
 * @date 2024-09-04
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __FILES_ERROR_H
#define __FILES_ERROR_H

/**
 * @brief Maximum number of tags that can be attached to file
 */
#define FILE_MAX_TAGS 8

/**
 * @brief Success status of file operation
 */
enum FileError {
  FERR_NONE,              /*!< No error */
  FERR_INVALID_VALUE,     /*!< Invalid parameter value */
  FERR_INVALID_OPERATION, /*!< Operation is not allowed */
  FERR_ACCESS_DENIED      /*!< Denied access to file */
};

typedef int file_error_t;

/**
 * @brief Convert file error code to human-readable string
 *
 * @return Error message string
 */
const char* file_error_to_string(file_error_t error);

#endif /* Error.h */ 
