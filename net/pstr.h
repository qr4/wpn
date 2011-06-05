
//
// code by twist<at>nerd2nerd.org
//

#ifndef __PSTR_H
#define __PSTR_H

#include <stddef.h>

struct pstr {
  size_t size;
  size_t used;
  char str[256];
};

int pstr_append(struct pstr* dest, char* src, int src_len);

#endif

