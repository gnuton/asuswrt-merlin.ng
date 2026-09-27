/* Reuse the independently trapped service boundaries with actual sdn.o. */
#define main default_fixture_unused_main
#include "sdn-default-driver.c"
#undef main

int main(int argc, char **argv)
{
	int result;
	assert(argc == 2 && !strcmp(argv[1], "1"));
	result = handle_sdn_feature(1, SDN_FEATURE_WAN, 0);
	printf("Actual broad-quarantine SDN correction: result %d\n", result);
	return result ? 1 : 0;
}
