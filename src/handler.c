#include "cmd.h"
#include "init.h"
#include "handler.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#ifdef __APPLE__
  #define OPEN_CMD "open"
#else
  #define OPEN_CMD "xdg-open"
#endif

#define MAX_ALIAS_LEN 128

static int add_document (const char* type, const char* alias, const char* location);
static int list_documents (void);
static void open_document (const char* name);

// Helper functions
static int valid_entries(const char* alias, const char* type, const char* location);
static int get_file_location(char* file_location_buffer, const char* alias);

void handle_command(const Commands command, int argc, const char* argv[]) {

  switch (command) {
    case CMD_INVALID: {
      fprintf(stderr, "Unknow command.\n");
      break;
    }

    case CMD_ADD: {
      if (argc < 5) {
        fprintf(stderr, "Usage: ./qrd -a <document type> <alias> <file location>.\n");
        break;
      }

      if (add_document(argv[2], argv[3], argv[4])) {
        printf("%s added successfully.\n", argv[2]);
      } else {
        fprintf(stderr, "Failed to add document.\n");
      }
      break;
    }

    case CMD_OPEN: {
      if (argc != 3) {
        fprintf(stderr, "Usage: ./qrd -o <alias>.\n");
        break;
      }

      open_document(argv[2]);
      break;
    }

    default:
      fprintf(stderr, "Valid command.\n");
  }

}

static int add_document (const char* type, const char* alias, const char* location) {
  const char* registry = get_registry_path();

  if (!valid_entries(alias, registry, location)) {
    fprintf(stderr, "Invalid entries.\n");
    return 0;
  }

  FILE* fp = fopen(registry, "a");

  if (!fp) {
    fprintf(stderr, "Unable to open registry path.\n");
    return 0;
  }

  const unsigned int type_len = strlen(type) + 1;

  char lower_case_type[type_len];
  for (unsigned int i = 0; i < type_len; i++) {
    lower_case_type[i] = (char)tolower((unsigned char)type[i]);
  }
  lower_case_type[type_len - 1] = '\0';

  fprintf(fp, "%s:%s:%s;\n", lower_case_type, alias, location);

  fclose(fp);
  return 1;
}

static void open_document(const char* alias) {
 
  char location_buffer[PATH_MAX];
  if (!get_file_location(location_buffer, alias)) {
    return;
  } 

  int status = 0;
  pid_t pid = fork();

  if (pid == 0) {
    char* exe_command[] = {OPEN_CMD, location_buffer, NULL};
    execvp(OPEN_CMD, exe_command);
  } else {
    wait(&status);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      fprintf(stderr, "Can't open %s.\n", alias);
      return;
    } 
  }

  fprintf(stdout, "Opened: %s\n", alias);
}

static int valid_entries(const char* alias, const char* type, const char* location) {

  const char* illegal = ":;";

  if (strpbrk(alias, illegal) || strpbrk(type, illegal) || strpbrk(location, illegal)) {
    fprintf(stderr, "qrd error: Input contains forbidden characters (':' or ';')\n");
    return 0;
  }

  return 1;
}

static void copy_buffer(char* des, char* src, char delimiter) {
  size_t i = 0;
  while (src[i] != delimiter) {
    des[i] = src[i];
    i++;
  }
  des[i] = '\0';
}

static int get_file_location(char* file_location_out, const char* target_alias) {
  const char* registry_path = get_registry_path();
  FILE* fp = fopen(registry_path, "r");
  if (!fp) return 0;

  char* line = NULL;
  size_t line_limit = 0;

  while (getline(&line, &line_limit, fp) != -1) {
    // Find 1st colon
    char* p = strchr(line, ':');
    if (!p) continue;

    char current_alias[128]; 
    p++; 
    copy_buffer(current_alias, p, ':');

    if (strcmp(current_alias, target_alias) == 0) {
      // Find 2nd colon (search starting from the alias)
      p = strchr(p, ':');
      if (p) {
        p++; 
        copy_buffer(file_location_out, p, ';');

        free(line);
        fclose(fp);
        return 1;
      }
    }
  }

  fprintf(stderr, "No %s found.\n", target_alias);

  free(line);
  fclose(fp);
  return 0;
}
