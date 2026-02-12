#ifndef HANDLER_H
#define HANDLER_H

#include "cmd.h"

void handle_command(Commands command, int argc, const char* argv[]);
char* get_book_name(int argc, const char* argv[]);

#endif
