//
// code by twist<at>nerd2nerd.org
//

#define _GNU_SOURCE

#include <openssl/sha.h>  // fuer pw hashing

#include <sys/types.h>    // stat
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>       // strtol

#include <fcntl.h>        // open O_WRONLY, O_TRUNC, ... und so zeux

#include <logging.h>

#include "userstuff.h"
#include "config.h"



// schaut unter USER_HOME_BY_NAME/$name nach ob es ein verzeichnis (oder eine andere datei) gibt 
// 1 = ja, 0 = nein
int have_user(struct pstr* name) {
  struct pstr path = { .used = sizeof(USER_HOME_BY_NAME), .str = USER_HOME_BY_NAME "/" };
  pstr_append(&path, name);

  struct stat sb;

  if (stat(pstr_as_cstr(&path), &sb) == -1) {
    if (errno != ENOENT) {
      log_perror("stat");
    }
    return 0;
  }

  if ((sb.st_mode & S_IFMT) != S_IFDIR) {
    log_msg("path %s is not a directory - why??", path.str);
  }

  return 1;
}


int get_id(struct pstr* file) {
  struct pstr id = { .used = 0 };

  int ret = -1;

  if (pstr_read_file(file, &id) == -1) goto err;

  errno = 0;
  ret = strtol(pstr_as_cstr(&id), (char **) NULL, 10);
  if (errno != 0) goto err;

  return ret;

err:
  log_msg("probleme beim lesen von %s", pstr_as_cstr(file));
  log_perror("get_id");
  return -1;
}


int set_id(struct pstr* file, int id) {
  struct pstr data = { .used = 0 };
  pstr_append_printf(&data, "%d\n", id);

  if (pstr_write_file(file, &data, O_WRONLY | O_CREAT | O_TRUNC) == -1) goto err;

  return 0;

err:
  log_msg("probleme beim schreiben von %s", pstr_as_cstr(file));
  log_perror("set_id");
  return -1;
}


void crypt_passwd(const void* pw, unsigned long pw_len, struct pstr* hash) {
  pstr_clear(hash);
  SHA_CTX c;
  SHA1_Init(&c);
  SHA1_Update(&c, pw, pw_len);
  SHA1_Final((unsigned char *)hash->str, &c);
  hash->used = SHA_DIGEST_LENGTH;
  hash->str[SHA_DIGEST_LENGTH] = '\0';
}

// legt einen neuen user unter USER_HOME_BY_ID/$id und USER_HOME_BY_NAME/$name (symlink) 
// mit password $pw an. gibt ihm eine neue id (wird auch zurueckgegeben)
// bei "geht nicht" wird -1 zurueckgegeben
int add_user(char *name, int len_name, const void* pw, unsigned long pw_len, unsigned int* user_id) {

  struct pstr home_by_id = { .used = sizeof(USER_HOME_BY_ID), .str = USER_HOME_BY_ID "/" };
  struct pstr home_by_name = { .used = sizeof(USER_HOME_BY_NAME), .str = USER_HOME_BY_NAME "/" };
  struct pstr file;

  {
    // neue user-id bauen
    struct pstr file_last_id = { .used = sizeof(CONFIG_LAST_ID) - 1, .str = CONFIG_LAST_ID };
    if ((*user_id = get_id(&file_last_id)) == -1) return -1;

    (*user_id)++;

    if (set_id(&file_last_id, *user_id) == -1) {
      log_perror("kann "CONFIG_LAST_ID" nicht schreiben!");
      return -1;
    }
  }

  pstr_append_printf(&home_by_id, "%d", *user_id);
  pstr_append_cstr(&home_by_name, name, len_name);

  {
    // verzeichnis und symlink fuer den user anlegen
    if (mkdir(pstr_as_cstr(&home_by_id), 0700) == -1) {
      log_perror("mkdir");
      return -1;
    }
    if (symlink(pstr_as_cstr(&home_by_id), pstr_as_cstr(&home_by_name)) == -1) {
      log_perror("symlink home-by-name");
      return -1;
    }
  }

  {
    // user id schreiben
    pstr_set(&file, &home_by_id);
    pstr_append_cstr(&file, "/id", 3);

    struct pstr id = { .used = 0 };
    pstr_append_printf(&id, "%d\n", *user_id);

    if (pstr_write_file(&file, &id, O_CREAT | O_EXCL | O_WRONLY) == -1) {
      log_perror("write user-id");
      log_msg("write mag id nicht schreiben. id = %s", pstr_as_cstr(&id));
      return -1;
    }
  }

  {
    // user name schreiben
    pstr_set(&file, &home_by_id);
    pstr_append_cstr(&file, "/name", 5);

    struct pstr user_name = { .used = 0 };
    pstr_append_cstr(&user_name, name, len_name);

    if (pstr_write_file(&file, &user_name, O_CREAT | O_EXCL | O_WRONLY) == -1) {
      log_perror("write user-name");
      log_msg("write mag name nicht schreiben. id = %s", name);
      return -1;
    }
  }

  {
    // passwd schreiben
    struct pstr hash;
    crypt_passwd(pw, pw_len, &hash);

    pstr_set(&file, &home_by_id);
    pstr_append_cstr(&file, "/passwd", 7);

    if (pstr_write_file(&file, &hash, O_CREAT | O_EXCL | O_WRONLY) == -1) {
      log_perror("pw-file write");
      return -1;
    }
  }

  return 0;
}



