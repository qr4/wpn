
//
// code by twist<at>nerd2nerd.org
//

#include "pstr.h"

#include <string.h>
#include <stdlib.h>

//
// pstr (sowas wie pascal-strings nur mit fixer groesse)
//

int pstr_append(struct pstr* dest, char* src, int src_len) {
  size_t len = src_len + dest->used > dest->size ? dest->size - dest->used : src_len;
  memcpy(dest->str + dest->used, src, len);
  dest->used += len;
  return len;
}


//
// dstr (sowas wie pascal-strings - hier mit dynamischer groesse)
//


// init dstr. malloc und sowas
int pstr_malloc(struct dstr* p) {
  p->data = (char*)malloc(PSTR_DSTR_DEFAULT_SIZE);
  if (p->data == NULL) {
    return -1;
  }
  p->size = PSTR_DSTR_DEFAULT_SIZE;
  p->unused = 0;
  p->next = 0;
  return 0;
}

// dstr wieder frei geben
void pstr_free(struct dstr* p) {
  free(p->data);
}

// inhalt loeschen
void pstr_clear(struct dstr* p) {
  p->unused = 0;
  p->next = 0;
}

// dstr als \0-terminierten string zurueckgeben = bereich von next bis unused 
char* dstr_as_cstr(struct dstr* p) {
  return p->data + p->next;
}

// fuegt src mit der laenge src_len an dstr an
int dstr_append(struct dstr* p, char* src, size_t src_len) {
  // da wir data mit \0 terminieren wollen muessen wir
  // src_len bei der speicherberechnung um +1 nach oben korrigieren

  // passt so wohl nicht mehr rein
  if ((src_len + 1) + p->unused > p->size) {
    // vorne ist noch was frei. evtl. reicht das ja
    if (p->next != 0) {
      memmove(p->data, p->data + p->next, p->unused - p->next);
      p->unused -= p->next;
      p->next = 0;
    }
    
    // passt wohl immer noch nicht -> realloc
    if ((src_len + 1) + p->unused > p->size) {
      size_t new_size = (src_len + 1) > p->size ? p->size + (src_len + 1) : p->size * 2;
      char* tmp = (char*)realloc(p->data, new_size);
      if (tmp == NULL) {
        return -1;
      }
      p->data = tmp;
      p->size = new_size;
    }
  }

  // so, hier haben wir jetzt genug platz
  memcpy(p->data + p->unused, src, src_len);
  p->unused += src_len;
  p->data[p->unused] = '\0';

  return 0;
}

// wie oebn, nur hier ist src \0-terminiert
int dstr_append_cstr(struct dstr* p, char* src) {
  return dstr_append(p, src, strlen(src));
}

// gibt die naechste zeile zurueck
// < 0, wenn kein \n bzw. \r oder sowas gefunden wurde
int dstr_read_line(struct dstr* p, char** data, int* len) {
  if (p->next == p->unused) {
    return -1;
  }

  // komische konstellation wie \n\r als nur einen zeilenumbruch handeln
  if ( ((p->last_char == '\n') && (p->data[p->next] == '\r')) ||
      ((p->last_char == '\r') && (p->data[p->next] == '\n')) ) {
    p->next++;
  }

  for (size_t i = p->next; i < p->unused; ++i) {
    if ( (p->data[i] == '\n') || (p->data[i] == '\r') ) {
      (*len) = i - p->next;
      (*data) = p->data + p->next;
      p->last_char = p->data[i];
      p->next = i + 1;
      return 0;
    }
  }

  return -1;
}


