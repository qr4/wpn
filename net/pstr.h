
//
// code by twist<at>nerd2nerd.org
//

#ifndef __PSTR_H
#define __PSTR_H

#include <stddef.h>

#define PSTR_DSTR_DEFAULT_SIZE 4000

struct pstr {
  size_t size;
  size_t used;
  char str[256];
};

int pstr_append(struct pstr* dest, char* src, int src_len);


// nutzdaten: #
//
// data -> |-----############---------------------------|
//               ^           ^- unused
//               \- next
struct dstr {
  char* data;     // malloc prt
  size_t size;    // groesse von malloc/realloc
  size_t unused;  // 1. freie stelle nach den nutzdaten
  size_t next;    // start der nutzdaten
  char last_char; // letztes gelesenes zeichen - brauchen wir fuer zeilenumbrucherkennung
};

int pstr_malloc(struct dstr* p);
void pstr_free(struct dstr* p);
void pstr_clear(struct dstr* p);
char* dstr_as_cstr(struct dstr* p);
int dstr_append(struct dstr* p, char* src, size_t src_len);
int dstr_append_cstr(struct dstr* p, char* src);
int dstr_read_line(struct dstr* p, char** data, int* len);


#endif

