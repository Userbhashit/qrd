#include "init.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Helper function
static void ensure_config_dir(const char* dir_path);

const char* get_registry_path(void) {
  return registry_path;
}

void init_qrd(void) {
  char config_dir[PATH_MAX];
  char dir[PATH_MAX];
  char* home = getenv("HOME");

  if (!home) {
    fprintf(stderr, "Error: $HOME environment variable not set.\n");
    exit(INIT_FAIL);
  }

  // Ensure ~/.config exists else create it
  snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
  ensure_config_dir(config_dir);

  // Ensure ~/.config/qrd exists else create it
  snprintf(dir, sizeof(dir), "%s/qrd", config_dir);
  ensure_config_dir(dir);

  snprintf(registry_path, sizeof(registry_path), "%s/registry", dir);

  int fd = open(registry_path, O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (fd == -1) {
    perror("qrd: could not create registry file");
    exit(INIT_FAIL);
  }

  close(fd);
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
