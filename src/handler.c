#include "cmd.h"
#include "init.h"
#include "handler.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>

#ifdef __APPLE__
  #define OPEN_CMD "open"
#else
  #define OPEN_CMD "xdg-open"
#endif

static int add_document (const char* type, const char* alias, const char* location);
static int list_documents (void);
static int open_document (const char* name);

// Helper functions
int valid_entries(const char* alias, const char* type, const char* location);

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

static int open_document(const char* name) {

  int status = 0;
  pid_t pid = fork();

  if (pid == 0) {

    char* exe_command[] = {OPEN_CMD, (char*)name, NULL};
    execvp(OPEN_CMD, exe_command);
  } else {
    wait(&status);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      fprintf(stderr, "Can't open %s.\n", name);
    } else {
      printf("open\n");
    }
  }

  return 0;
}

int valid_entries(const char* alias, const char* type, const char* location) {

  const char* illegal = ":;";

  if (strpbrk(alias, illegal) || strpbrk(type, illegal) || strpbrk(location, illegal)) {
    fprintf(stderr, "qrd error: Input contains forbidden characters (':' or ';')\n");
    return 0;
  }

  return 1;
}
