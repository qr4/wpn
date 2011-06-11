
//
// code by twist<at>nerd2nerd.org
//

#include "pstr.h"

#include <string.h>
#include <stdlib.h>

//
// pstr (sowas wie pascal-strings nur mit fixer groesse)
//

// pstr leer machen
void pstr_clear(struct pstr* p) {
  p->used = 0;
}

// pstr als char*
char* pstr_as_cstr(struct pstr* p) {
  return p->str;
}

void pstr_set(struct pstr* dest, struct pstr* src) {
  memcpy(dest->str, src->str, src->used + 1);
  dest->used = src->used;
}

// setzt src als string
int pstr_set_cstr(struct pstr* p, char* src, int src_len) {
  const size_t str_size = sizeof ((struct pstr*)0)->str - 1;  // -1 wegen \0-terminierung
  size_t len = str_size < src_len ? str_size : src_len;
  memcpy(p->str, src, len);
  p->used = len;
  p->str[len] = '\0';
  return len;
}

// an den pstr was drannhaengen
int pstr_append(struct pstr* dest, char* src, int src_len) {
  const size_t str_size = sizeof ((struct pstr*)0)->str - 1;  // -1 wegen \0-terminierung
  size_t len = src_len + dest->used > str_size ? str_size - dest->used : src_len;
  memcpy(dest->str + dest->used, src, len);
  dest->used += len;
  dest->str[dest->used] = '\0';
  return len;
}

// laenge von dem pstr abfragen
int pstr_len(struct pstr* p) {
  return p->used;
}




//
// dstr (sowas wie pascal-strings - hier mit dynamischer groesse)
//


// init dstr. malloc und sowas
int dstr_malloc(struct dstr* p) {
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
void dstr_free(struct dstr* p) {
  free(p->data);
}

// inhalt loeschen und puffer-groesse auf ein vertraegliches mass
// zuruecksetzen (wenn noetig)
void dstr_clear(struct dstr* p) {
  p->unused = 0;
  p->next = 0;
  if (p->size > 800000) {
    char* tmp = realloc(p->data, 500000);
    if (tmp != NULL) {
      p->data = tmp;
    }
  }
}

// dstr als \0-terminierten string zurueckgeben = bereich von next bis unused 
char* dstr_as_cstr(struct dstr* p) {
  return p->data + p->next;
}

// setzt src als string
int dstr_set(struct dstr* p, char* src, size_t src_len) {
  size_t size = p->size;

  if (size < src_len + 1) {
    size = size * 2 > (src_len + 1) ? size * 2 : (src_len + 1); // + 1 wegen \0-terminierung
    char* tmp = (char*)realloc(p->data, size);
    if (tmp == NULL) {
      return -1;
    }
    p->data = tmp;
    p->size = size;
  }
  memcpy(p->data, src, src_len);
  p->unused = src_len;
  p->next = 0;
  p->last_char = '\0';
  p->data[src_len] = '\0';
  return 0;
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

// gibt die anzahl der belegten bytes zureuck
int dstr_len(struct dstr* p) {
  return p->unused - p->next;
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


