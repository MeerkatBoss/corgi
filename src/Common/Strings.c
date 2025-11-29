#include "Strings.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

char* copy_string(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  size_t len = strlen(str) + 1;
  char* copy = calloc(len, 1);
  if (copy != NULL) {
    memcpy(copy, str, len);
  }
  return copy;
}

unsigned long append_string(
  char* buf,
  unsigned long buf_size,
  const char* str
) {
  unsigned long orig_len = strlen(buf);
  unsigned long str_len = strlen(str);
  assert(orig_len < buf_size);

  unsigned long remaining_len = buf_size - orig_len - 1;
  unsigned long copy_len = (str_len < remaining_len) ? str_len : remaining_len;

  strncpy(buf + orig_len, str, copy_len);
  buf[orig_len + copy_len] = '\0';

  return orig_len + str_len;
} 