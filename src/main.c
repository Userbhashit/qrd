#include "cmd.h"
#include "init.h"
#include "handler.h"

int main(int argc, const char **argv) {

  const Commands cmd = get_command(argc, argv);
  init_qrd(cmd);
  handle_command(cmd, argc, argv);

  return 0;
}
