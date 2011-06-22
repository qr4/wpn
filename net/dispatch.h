//
// code by twist<at>nerd2nerd.org
//

#ifndef DISPATCH_H
#define DISPATCH_H

//
// wird fuer die kommunikation via pipe verwendet um sicherzustellen, dass alle daten da sind
//
struct pipe_com {
  union {
    struct {
      unsigned int id;
      int len;
    } head;
    char _head[0];
  } header;
  int read_head;
  int read_body;
};

int dispatch(struct pipe_com* pc, int read_pipe, char** data, int* data_len);

#endif


