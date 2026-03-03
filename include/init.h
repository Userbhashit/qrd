#ifndef INIT_H
#define INIT_H

#define INIT_FAIL 1

#include "cmd.h"

void init_qrd(const Commands cmd);
const char* get_registry_path(void);
const char* get_viewer_command(void);

#endif
