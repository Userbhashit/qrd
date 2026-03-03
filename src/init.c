#include "init.h"
#include "cmd.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


// Helper function
static int set_viewer_command(char* config_dir_path);
static void ensure_config_dir(const char* dir_path);
static void print_viewer_command_error(void);

static char* registry_path;
const char* get_registry_path(void) {
  return registry_path;
}

static char* viewer_command;
const char* get_viewer_command(void) {
  return viewer_command;
}

void free_registry_path_and_viewer_command(void) {
  if(registry_path) {
    free(registry_path);
    registry_path = NULL;
  }

  if(viewer_command) {
    free(viewer_command);
    viewer_command = NULL;
  }
}

void init_qrd(const Commands cmd) {
  char* home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "Error: $HOME environment variable not set.\n");
    exit(INIT_FAIL);
  }

  const char* reg_suffix = "/.config/qrd/registry";
  size_t path_len = strlen(home) + strlen(reg_suffix) + 1; // Extra one for '\0'

  registry_path = malloc(path_len);
  if (!registry_path) {
    perror("malloc registry_path");
    exit(INIT_FAIL);
  }  

  atexit(free_registry_path_and_viewer_command);

  // Ensure ~/.config exists
  snprintf(registry_path, path_len, "%s/.config", home);
  ensure_config_dir(registry_path);

  // Ensure ~/.config/qrd exists
  snprintf(registry_path, path_len, "%s/.config/qrd", home);
  ensure_config_dir(registry_path);

  if (cmd != CMD_SET_COMMAND && !set_viewer_command(registry_path)) {
    print_viewer_command_error();
    return;
  }

  snprintf(registry_path, path_len, "%s%s", home, reg_suffix);

  int fd = open(registry_path, O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (fd == -1) {
    perror("qrd: could not create registry file");
    free(registry_path); 
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

static void print_viewer_command_error(void) {
  fprintf(stderr, 
      "Error: No viewer command configured.\n\n"
      "To fix this, set your preferred viewer (e.g., 'open', 'xdg-open', or 'cat'):\n"
      "    qrd -s <command_name>\n\n"
      "Use 'qrd -h' for more information.\n"
      );
}

static int set_viewer_command(char* config_dir_path) {
  char* viewer_file_name = "/command";
  size_t path_len = strlen(config_dir_path) + strlen(viewer_file_name) + 1;
  
  char* viewer_command_path = malloc(path_len);
  if (!viewer_command_path) {
    fprintf(stderr, "Failed to allocate viewer command path.\n");
    exit(INIT_FAIL);
  }

  snprintf(viewer_command_path, path_len, "%s%s", config_dir_path, viewer_file_name);

  FILE* fp = fopen(viewer_command_path, "r");
  free(viewer_command_path);

  if (!fp) {
    return 0;
  }

  size_t line_limit = 0;
  ssize_t nread = getline(&viewer_command, &line_limit, fp);
  if (nread == -1) {
    return 0;
  }

  if (nread > 0 && viewer_command[nread - 1] == '\n') {
    viewer_command[nread - 1] = '\0';
  }

  return 1;
}
