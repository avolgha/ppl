#define NOB_IMPLEMENTATION
#include "nob.h"

int
main (int argc, char **argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);

	if (!nob_mkdir_if_not_exists("dist"))
		return 1;

	Nob_Cmd cmd = {0};
	nob_cc(&cmd);
	nob_cc_flags(&cmd);
	nob_cmd_append(&cmd, "-l", "sqlite3", "-l", "readline");
	nob_cc_output(&cmd, "dist/ppl");
	nob_cc_inputs(&cmd, "ppl.c");

	if (!nob_cmd_run_sync_and_reset(&cmd))
		return 1;
	else
		return 0;
}
