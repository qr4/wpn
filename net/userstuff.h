//
// code by twist<at>nerd2nerd.org
//

#ifndef USERSTUFF_H
#define USERSTUFF_H

#include "pstr.h"

int have_user(struct pstr* name);
int get_id(struct pstr* file);
void crypt_passwd(const void* pw, unsigned long pw_len, struct pstr* hash);
int add_user(char *name, int len_name, const void* pw, unsigned long pw_len, unsigned int* user_id);

#endif




