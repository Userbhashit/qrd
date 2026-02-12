#include "cmd.h"

#include <ctype.h>

static struct CommandInfo {
  char flag;
  Commands command;
} flags[] = {
  {'a', CMD_ADD},
  {'l', CMD_LIST},
  {'h', CMD_HELP},
  {'o', CMD_OPEN},
  {'d', CMD_REMOVE},
};

static int flags_len = sizeof flags / sizeof flags[0];

Commands get_command(int argc, const char* argv[]) {
  
  if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0') 
    return CMD_INVALID;

  char input_flag = (char)tolower(argv[1][1]);

  for (int i = 0; i < flags_len; i++) {
    if (flags[i].flag == input_flag) {
      return flags[i].command;
    }
  }

  return CMD_INVALID;
}
