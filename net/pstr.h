
//
// code by twist<at>nerd2nerd.org
//

#ifndef __PSTR_H
#define __PSTR_H

#include <stddef.h>

#define PSTR_DSTR_DEFAULT_SIZE 4000

struct pstr {
//  size_t size;
  size_t used;
  char str[512];
};

void pstr_clear(struct pstr* p);

void pstr_set(struct pstr* dest, struct pstr* src);
int pstr_set_cstr(struct pstr* p, char* src, int src_len);

int pstr_append(struct pstr* dest, struct pstr* src);
int pstr_append_cstr(struct pstr* dest, char* src, int src_len);

char* pstr_as_cstr(struct pstr* p);
int pstr_len(struct pstr* p);


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

int dstr_malloc(struct dstr* p);
void dstr_free(struct dstr* p);
void dstr_clear(struct dstr* p);
char* dstr_as_cstr(struct dstr* p);
int dstr_set(struct dstr* p, char* src, size_t src_len);
int dstr_append(struct dstr* p, char* src, size_t src_len);
int dstr_append_cstr(struct dstr* p, char* src);
int dstr_len(struct dstr* p);
int dstr_read_line(struct dstr* p, char** data, int* len);


#endif

