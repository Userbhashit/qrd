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

/* Copy characters from src into dest until delimiter or '\0', with bounds checking.
 * Returns 1 on success (delimiter found), 0 on failure. */
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
    /* Delimiter not found before end-of-string or buffer limit */
    dest[0] = '\0';
    return 0;
  }

  dest[i] = '\0';
  return 1;
}

/* Parse a single registry line into type, alias, and location.
 * Returns 1 on success, 0 on parse failure. */
static int parse_registry_line(char* line, char* type_out, char* alias_out, char* location_out) {
  char* delimiter = ":;";
  if (!strpbrk(line, delimiter)) {
    return 0;
  }

  /* type is everything up to first ':' */
  if (!copy_field(type_out, MAX_LEN, line, ':')) {
    return 0;
  }

  /* alias is between first and second ':' */
  char* p = strchr(line, ':');
  if (!p) return 0;
  ++p;
  if (!copy_field(alias_out, MAX_LEN, p, ':')) {
    return 0;
  }

  /* location is between second ':' and ';' */
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
      /* Copy full location out and return */
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

typedef struct {
  char type[MAX_LEN];
  char alias[MAX_LEN];
  char location[PATH_MAX];
} RegistryEntry;

static void list_documents(const char* filter_type) {
  const char* registry_path = get_registry_path();
  FILE* fp = fopen(registry_path, "r");
  if (!fp) {
    fprintf(stderr, "Unable to open registry file.\n");
    return;
  }

  /* Optional type filter, normalized to lowercase */
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

  RegistryEntry* entries = NULL;
  size_t entries_count = 0;
  size_t entries_capacity = 0;

  char* line = NULL;
  size_t line_limit = 0;

  /* Start with header widths so header text always fits */
  size_t max_type_len = strlen("Type");
  size_t max_alias_len = strlen("Alias");
  size_t max_location_len = strlen("Location");

  /* Single pass: parse entries, apply optional filter, track widths, and store them */
  while (getline(&line, &line_limit, fp) != -1) {
    RegistryEntry entry;

    if (!parse_registry_line(line, entry.type, entry.alias, entry.location)) {
      continue;
    }

    if (type_filter && strcmp(entry.type, type_filter) != 0) {
      continue;
    }

    size_t type_len = strlen(entry.type);
    size_t alias_len = strlen(entry.alias);
    size_t location_len = strlen(entry.location);

    if (type_len > max_type_len) max_type_len = type_len;
    if (alias_len > max_alias_len) max_alias_len = alias_len;
    if (location_len > max_location_len) max_location_len = location_len;

    if (entries_count == entries_capacity) {
      size_t new_capacity = entries_capacity ? entries_capacity * 2 : 8;
      RegistryEntry* new_entries = realloc(entries, new_capacity * sizeof(RegistryEntry));
      if (!new_entries) {
        fprintf(stderr, "qrd: out of memory while listing documents.\n");
        free(entries);
        free(line);
        fclose(fp);
        return;
      }
      entries = new_entries;
      entries_capacity = new_capacity;
    }

    entries[entries_count++] = entry;
  }

  free(line);
  fclose(fp);

  if (entries_count == 0) {
    if (type_filter) {
      printf("No documents found for type '%s'.\n", type_filter);
    } else {
      printf("No documents found.\n");
    }
    free(entries);
    return;
  }

  int type_width = (int)max_type_len;
  int alias_width = (int)max_alias_len;
  int location_width = (int)max_location_len;

  // Print table header 
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

  for (size_t i = 0; i < entries_count; ++i) {
    printf("| %-*s | %-*s | %-*s |\n", type_width, entries[i].type, alias_width, entries[i].alias, location_width, entries[i].location);
  }

  printf("+");
  for (int i = 0; i < type_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < alias_width + 2; ++i) putchar('-');
  printf("+");
  for (int i = 0; i < location_width + 2; ++i) putchar('-');
  printf("+\n");

  free(entries);
}
