#ifndef CMD_H
#define CMD_H

typedef enum Commands {
  CMD_ADD,
  CMD_HELP,
  CMD_OPEN,
  CMD_LIST,
  CMD_REMOVE,
  CMD_INVALID,
} Commands;

Commands get_command(int argc, const char* argv[]);

#endif
