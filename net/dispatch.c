//
// code by twist<at>nerd2nerd.org
//

#include <unistd.h>   // read

#include <logging.h>  // log_

#include "dispatch.h"


//
// pc: pointer auf pipe_com zum puffern von header-infos
// read_pipe: fd aus welcher pipe gelesen werden soll
// data*: wurden erfolgreich daten gelesen zeigt data darauf, ist \0-terminiert
// data_len*: anzahl der zur verfuegung stehenden daten (0 = ist okay, halt keine daten)
// return -1: broken pipe oder verbindung unterbrochen
//        0...n: anzahl der noch ausstehenden bytes, wenn data_len != 0 
//        (-> kopf wurde schon uebertragen)
// ACHTUNG: dispatch puffert nicht, sondern gibt die bytes zurueck die gerade da sind
//          -> selber puffern wenn man die gesammte message haben will
//
int dispatch(struct pipe_com* pc, int read_pipe, char** data, int* data_len) {
  if (pc->read_head < sizeof((struct pipe_com*)0)->header) {
    // "header" noch nicht vollstaendig
    ssize_t read_len = read(read_pipe,
        pc->header._head + pc->read_head,   // header-position bestimmen, evtl. append
        sizeof((struct pipe_com*)0)->header - pc->read_head); // nicht ueber den kopf hinaus lesen
    if (read_len == -1) { log_perror("read from pipe"); return -1; }
    if (read_len == 0) { log_msg("keine verbindung zum vater (2)"); return -1; }
    pc->read_head += read_len;
    pc->read_body = 0;
    *data = 0;
    *data_len = 0;
    return 0;
  }

  // so, nutzdaten lesen und weiter schicken
  static char buffer[4000];
  int buffer_size = (sizeof(buffer) - 1) < pc->header.head.len - pc->read_body
    ? (sizeof(buffer) - 1)
    : pc->header.head.len - pc->read_body;
 
  ssize_t read_len = read(read_pipe, buffer, buffer_size);
  if (read_len == -1) { log_perror("read from pipe"); return -1; } 
  if (read_len == 0) { log_msg("keine verbindung zum vater (2)"); return -1; }
  pc->read_body += read_len;

  buffer[read_len] = '\0';

  // alles an den client weiter gegeben
  if (pc->read_body == pc->header.head.len) {
    pc->read_head = 0;
  }
 
  *data = buffer;
  *data_len = buffer_size;
 
  return pc->header.head.len - pc->read_body;
}

