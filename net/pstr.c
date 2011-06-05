
//
// code by twist<at>nerd2nerd.org
//

#include "pstr.h"

#include <string.h>


int pstr_append(struct pstr* dest, char* src, int src_len) {
  size_t len = src_len + dest->used > dest->size ? dest->size - dest->used : src_len;
  memcpy(dest->str + dest->used, src, len);
  dest->used += len;
  return len;
}



