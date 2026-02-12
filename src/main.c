#include "cmd.h"
#include "init.h"
#include "handler.h"

int main(int argc, const char **argv) {

  init_qrd();

  const Commands cmd = get_command(argc, argv);
  handle_command(cmd, argc, argv);

  return 0;
}
