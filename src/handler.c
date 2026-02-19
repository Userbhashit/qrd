#include "cmd.h"
#include "init.h"
#include "handler.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#if defined(__APPLE__)
  #include <sys/syslimits.h>
  #define OPEN_CMD "open"
#elif defined(__linux__)
  #include <limits.h>
  #include <linux/limits.h>
  #define OPEN_CMD "xdg-open"
#else 
  #error "This application is only supported for linux and macOS."
#endif

#define MAX_LEN 128

static int add_document (const char* type, const char* alias, const char* location);
static void open_document (const char* name);

// Helper functions
static int valid_entries(const char* alias, const char* type, const char* location);
static int get_file_location(char* file_location_buffer, const char* alias);
static void list_documents(const char* filter_type);
static void delete_document(const char* alias_to_delete);

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

    case CMD_LIST: {
      if (argc == 2) {
        list_documents(NULL);
      } else {
        list_documents(argv[2]);
      }
      break;
    }

    case CMD_REMOVE: {
      if (argc != 3) {
        fprintf(stderr, "Usage: ./qrd -d <alias_name>\n");
        return;
      }

      delete_document(argv[2]);
      break;
    }

    default:
      fprintf(stderr, "Invalid command.\n");
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

  if (strlen(alias) > MAX_LEN) {
    fprintf(stderr, "Alias cannot be more than 128 characters.\n");
    return 0;
  }

  if (strlen(type) > MAX_LEN) {
    fprintf(stderr, "Type cannot be more than 128 characters.\n");
    return 0;
  }
  const char* illegal = ":;";

  if (strpbrk(alias, illegal) || strpbrk(type, illegal) || strpbrk(location, illegal)) {
    fprintf(stderr, "qrd error: Input contains forbidden characters (':' or ';')\n");
    return 0;
  }

  return 1;
}

static int copy_field(char* dest, size_t dest_size, const char* src, char delimiter) {
  if (dest_size == 0) {
    return 0;
  }

  size_t i = 0;
  while (src[i] != '\0' && src[i] != delimiter && i + 1 < dest_size) {
    dest[i] = src[i];
    ++i;
  }

  if (src[i] != delimiter) {
    dest[0] = '\0';
    return 0;
  }

  dest[i] = '\0';
  return 1;
}

static int parse_registry_line(char* line, char* type_out, char* alias_out, char* location_out) {
  char* delimiter = ":;";
  if (!strpbrk(line, delimiter)) {
    return 0;
  }

  if (!copy_field(type_out, MAX_LEN, line, ':')) {
    return 0;
  }

  char* p = strchr(line, ':');
  if (!p) return 0;
  ++p;
  if (!copy_field(alias_out, MAX_LEN, p, ':')) {
    return 0;
  }

  p = strchr(p, ':');
  if (!p) return 0;
  ++p;
  if (!copy_field(location_out, PATH_MAX, p, ';')) {
    return 0;
  }

  return 1;
}

static int get_file_location(char* file_location_out, const char* target_alias) {
  const char* registry_path = get_registry_path();
  FILE* fp = fopen(registry_path, "r");
  if (!fp) return 0;

  char* line = NULL;
  size_t line_limit = 0;

  while (getline(&line, &line_limit, fp) != -1) {
    char type[MAX_LEN];
    char alias[MAX_LEN];
    char location[PATH_MAX];

    if (!parse_registry_line(line, type, alias, location)) {
      continue;
    }

    if (strcmp(alias, target_alias) == 0) {
      strncpy(file_location_out, location, PATH_MAX - 1);
      file_location_out[PATH_MAX - 1] = '\0';

      free(line);
      fclose(fp);
      return 1;
    }
  }

  fprintf(stderr, "No %s found.\n", target_alias);

  free(line);
  fclose(fp);
  return 0;
}

static void list_documents(const char* filter_type) {
  const char* registry_path = get_registry_path();
  FILE* fp = fopen(registry_path, "r");
  if (!fp) {
    fprintf(stderr, "Unable to open registry file.\n");
    return;
  }

  char normalized_type[MAX_LEN];
  const char* type_filter = NULL;
  if (filter_type) {
    size_t type_len = strlen(filter_type);
    if (type_len >= MAX_LEN) {
      fprintf(stderr, "Type cannot be more than %d characters.\n", MAX_LEN - 1);
      fclose(fp);
      return;
    }
    for (size_t i = 0; i < type_len; ++i) {
      normalized_type[i] = (char)tolower((unsigned char)filter_type[i]);
    }
    normalized_type[type_len] = '\0';
    type_filter = normalized_type;
  }

  char* line = NULL;
  size_t line_limit = 0;
  size_t max_type_len = strlen("Type");
  size_t max_alias_len = strlen("Alias");
  size_t max_location_len = strlen("Location");
  size_t entries_count = 0;

  while (getline(&line, &line_limit, fp) != -1) {
    char type[MAX_LEN];
    char alias[MAX_LEN];
    char location[PATH_MAX];

    if (!parse_registry_line(line, type, alias, location)) {
      continue;
    }

    if (type_filter && strcmp(type, type_filter) != 0) {
      continue;
    }

    size_t tl = strlen(type);
    size_t al = strlen(alias);
    size_t ll = strlen(location);
    if (tl > max_type_len) max_type_len = tl;
    if (al > max_alias_len) max_alias_len = al;
    if (ll > max_location_len) max_location_len = ll;
    entries_count++;
  }

  if (entries_count == 0) {
    if (type_filter) {
      printf("No documents found for type '%s'.\n", type_filter);
    } else {
      printf("No documents found.\n");
    }
    free(line);
    fclose(fp);
    return;
  }

  int type_width = (int)max_type_len;
  int alias_width = (int)max_alias_len;
  int location_width = (int)max_location_len;

  printf("+");
  for (int i = 0; i < type_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < alias_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < location_width + 2; ++i) putchar('-');
  printf("+\n");

  printf("| %-*s | %-*s | %-*s |\n", type_width, "Type", alias_width, "Alias", location_width, "Location");

  printf("+");
  for (int i = 0; i < type_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < alias_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < location_width + 2; ++i) putchar('-');
  printf("+\n");

  rewind(fp);
  while (getline(&line, &line_limit, fp) != -1) {
    char type[MAX_LEN];
    char alias[MAX_LEN];
    char location[PATH_MAX];

    if (!parse_registry_line(line, type, alias, location)) {
      continue;
    }

    if (type_filter && strcmp(type, type_filter) != 0) {
      continue;
    }

    printf("| %-*s | %-*s | %-*s |\n", type_width, type, alias_width, alias, location_width, location);
  }

  printf("+");
  for (int i = 0; i < type_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < alias_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < location_width + 2; ++i) putchar('-');
  printf("+\n");

  free(line);
  fclose(fp);
}

static void delete_document(const char* alias_to_delete) {
  const char* registry_path = get_registry_path();
  
  int backup_path_size = strlen(registry_path) + strlen(".backup") + 1; // extra 1 for '\0'
  char backup_path[backup_path_size];

  snprintf(backup_path, backup_path_size, "%s.backup", registry_path);

  if (rename(registry_path, backup_path) != 0) {
    fprintf(stderr, "Unable to create a backup file.\n");
    return;
  }

  FILE* backup_file_ptr = fopen(backup_path, "r");
  if (!backup_file_ptr) {
    rename(backup_path, registry_path);
    fprintf(stderr, "Unable to open backup file.\n");
    return;
  }

  FILE* new_registry_ptr = fopen(registry_path, "w");
  if(!new_registry_ptr) {
    rename(backup_path, registry_path);
    fprintf(stderr, "Unable to open new registry file.\n");
    return;
  }

  char* line = NULL;
  size_t line_limit = 0;

  int deleted = 0;
  while(getline(&line, &line_limit, backup_file_ptr) != -1) {
    char type[MAX_LEN];
    char alias[MAX_LEN];
    char location[MAX_LEN];

    parse_registry_line(line, type, alias, location);

    if (!deleted && strcmp(alias, alias_to_delete) == 0) {
      deleted = 1;
      continue;
    }

    fprintf(new_registry_ptr, "%s", line);
  }

  free(line);
  
  if (deleted) {
    fprintf(stdout, "Deleted: %s.\n", alias_to_delete);
  } else {
    fprintf(stdout, "%s not found.\n", alias_to_delete);
  }
}
