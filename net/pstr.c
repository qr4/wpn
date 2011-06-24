
//
// code by twist<at>nerd2nerd.org
//

#include "pstr.h"

#include <string.h>
#include <stdlib.h>

#include <sys/types.h>  // open, read, write, close
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//
// pstr (sowas wie pascal-strings nur mit fixer groesse)
//

// pstr leer machen
void pstr_clear(struct pstr* p) {
  p->used = 0;
  p->str[0] = '\0';
}


// setzte src als string
void pstr_set(struct pstr* dest, struct pstr* src) {
  memcpy(dest->str, src->str, src->used + 1);
  dest->used = src->used;
}

// setzt src als string
int pstr_set_cstr(struct pstr* p, const char* src, int src_len) {
  const size_t str_size = sizeof((struct pstr*)0)->str - 1;  // -1 wegen \0-terminierung
  size_t len = str_size < src_len ? str_size : src_len;
  memcpy(p->str, src, len);
  p->used = len;
  p->str[len] = '\0';
  return len;
}


// an den pstr was drannhaengen
int pstr_append(struct pstr* dest, struct pstr* src) {
  return pstr_append_cstr(dest, src->str, src->used);
}

// an den pstr was drannhaengen
int pstr_append_cstr(struct pstr* dest, const char* src, int src_len) {
  if (src_len == 0) return 0;

  const size_t str_size = sizeof ((struct pstr*)0)->str - 1;  // -1 wegen \0-terminierung
  size_t len = src_len + dest->used > str_size ? str_size - dest->used : src_len;
  memcpy(dest->str + dest->used, src, len);
  dest->used += len;
  dest->str[dest->used] = '\0';
  return len;
}

// an den pstr das zeux von printf drann haengen
int pstr_append_printf(struct pstr* p, const char* msg, ...) {
  va_list ap;
  va_start(ap, msg);

  const size_t str_size = sizeof ((struct pstr*)0)->str;  // kein -1 wegen \0-terminierung weil vsnprintf 0 terminiert
  int count = vsnprintf(p->str + p->used, str_size - p->used, msg, ap);

  p->used += count;

  if (count >= str_size - p->used) {
    // passt nicht mehr rein
    return -1;
  }

  return 0;
}

// vergleiche a und b; 0 = beide gleich
int pstr_cmp(struct pstr* a, struct pstr* b) {
  if (a->used == b->used) {
    return memcmp(a->str, b->str, a->used);
  }
  return 1;
}

// pstr als char*
char* pstr_as_cstr(struct pstr* p) {
  return p->str;
}

// laenge von dem pstr abfragen
int pstr_len(struct pstr* p) {
  return p->used;
}

// pstr in eine datei schreiben -> 0 wenns geklappt hat, sonst -1
int pstr_write_file(struct pstr* p, struct pstr* data, int flags) {
  int fd = open(pstr_as_cstr(p), flags, 0666);
  if (fd == -1) return -1;

  ssize_t count = write(fd, pstr_as_cstr(data), pstr_len(data));
  if (count == -1) {
    // das ging was mächtig schief. warum auch immer
    int _errno = errno;
    close(fd);
    errno = _errno;
    return -1;
  } else if (count != pstr_len(data)) {
    // hier sollte man noch den rest schreiben. kommt auf die TODO-liste
  }

  close(fd);
  return 0;
}

// datei-inhalt nach pstr schreiben -> 0 wenns geklappt hat, sonst -1
// -1 gibts auch, wenn der inhalt nicht rein passt
int pstr_read_file(struct pstr* p, struct pstr* data) {
  int fd = open(pstr_as_cstr(p), O_RDONLY, 0666);
  if (fd == -1) return -1;

  const ssize_t exact_buffer_size = sizeof((struct pstr*)0)->str;

  ssize_t count = read(fd, data->str, exact_buffer_size);

  if (count == -1) {
    // das ging was mächtig schief. warum auch immer
    data->used = 0;   // zur sicherheit "leeren"
    int _errno = errno;
    close(fd);
    errno = _errno;
    return -1;
  }

  if (count == exact_buffer_size) {
    // wir gehen davon aus, dass wir den inhalt der datei mit einem schwung lesen koennen
    // da von exact_buffer_size noch ein byte fuer \0 weg geht ist die datei zu gross
    close(fd);
    errno = EOVERFLOW;
    return -1;
  }

  data->used = count;
  data->str[count] = '\0';

  close(fd);
  return 0;
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
  p->data[0] = '\0';
  if (p->size > 800000) {
    char* tmp = realloc(p->data, 500000);
    if (tmp != NULL) {
      p->data = tmp;
    }
  }
}

// dstr als \0-terminierten string zurueckgeben = bereich von next bis ohne unused 
char* dstr_as_cstr(struct dstr* p) {
  return p->data + p->next;
}

// setzt src als string
int dstr_set(struct dstr* p, const char* src, size_t src_len) {
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
int dstr_append(struct dstr* p, const char* src, size_t src_len) {
  if (src_len == 0) return 0;

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

// wie oben, nur hier ist src \0-terminiert
int dstr_append_cstr(struct dstr* p, const char* src) {
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

// dstr in eine datei schreiben -> 0 wenns geklappt hat, sonst -1
int dstr_write_file(struct pstr* p, struct dstr* data, int flags) {
  int fd = open(pstr_as_cstr(p), flags, 0666);
  if (fd == -1) return -1;

  ssize_t count = write(fd, dstr_as_cstr(data), dstr_len(data));
  if (count == -1) {
    // das ging was mächtig schief. warum auch immer
    int _errno = errno;
    close(fd);
    errno = _errno;
    return -1;
  } else if (count != dstr_len(data)) {
    // hier sollte man noch den rest schreiben. kommt auf die TODO-liste
  }

  close(fd);
  return 0;
}

// datei-inhalt nach dstr schreiben -> 0 wenns geklappt hat, sonst -1
int dstr_read_file(struct pstr* p, struct dstr* data) {
  char buffer[4096];

  int fd = open(pstr_as_cstr(p), O_RDONLY, 0666);
  if (fd == -1) return -1;

  ssize_t count;

  do {
    count = read(fd, buffer, sizeof(buffer));
    if (count == -1) {
      // o_O ... die ist richtig was futsch
      int _errno = errno;
      close(fd);
      errno = _errno;
      return -1;
    }
    dstr_append(data, buffer, count);
  } while (count > 0);

  close(fd);
  return 0;
}


