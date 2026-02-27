#include "init.h"

#include <_stdio.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Helper function
static void ensure_config_dir(const char* dir_path);

static char* registry_path;
const char* get_registry_path(void) {
  return registry_path;
}

void free_registry_path(void) {
  if(registry_path) {
    free(registry_path);
    registry_path = NULL;
  }
}

void init_qrd(void) {
  char* home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "Error: $HOME environment variable not set.\n");
    exit(INIT_FAIL);
  }

  // Path parts
  const char* config_suffix = "/.config";
  const char* qrd_suffix = "/.config/qrd";
  const char* reg_suffix = "/.config/qrd/registry";

  size_t path_len = strlen(home) + strlen(reg_suffix) + 1;

  // Single local buffer for setup tasks
  char tmp_path[path_len];

  // Ensure ~/.config exists
  snprintf(tmp_path, path_len, "%s%s", home, config_suffix);
  ensure_config_dir(tmp_path);

  // Ensure ~/.config/qrd exists
  snprintf(tmp_path, path_len, "%s%s", home, qrd_suffix);
  ensure_config_dir(tmp_path);

  registry_path = malloc(path_len);
  if (!registry_path) {
    perror("malloc registry_path");
    exit(INIT_FAIL);
  }

  snprintf(registry_path, path_len, "%s%s", home, reg_suffix);

  int fd = open(registry_path, O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (fd == -1) {
    perror("qrd: could not create registry file");
    free(registry_path); 
    exit(INIT_FAIL);
  }

  close(fd);
  atexit(free_registry_path);
}

static void ensure_config_dir(const char* dir_path) {
  struct stat st;

  if (stat(dir_path, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      fprintf(stderr, "qrd: %s exists and is not a directory\n", dir_path);
      exit(INIT_FAIL);
    }
    return;
  }

  if (errno == ENOENT) {
    if (mkdir(dir_path, 0700) == -1) {
      perror("qrd: could not create config directory");
      exit(INIT_FAIL);
    }
    return;
  }

  perror("qrd: could not stat config directory");
  exit(INIT_FAIL);
}
