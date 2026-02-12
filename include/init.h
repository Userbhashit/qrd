#ifndef INIT_H
#define INIT_H

#define INIT_FAIL 1

#include <limits.h>

// Global registry path 
static char registry_path[PATH_MAX];

void init_qrd(void);
const char* get_registry_path(void);

#endif
