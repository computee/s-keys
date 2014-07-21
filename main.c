#include <jump_label.h>

int main (int argc, char *argv[])
{
	struct static_key key = STATIC_KEY_INIT_FALSE;

	if (static_key_false(&key))
		printf("false\n");
	else
		printf("true\n");

	return 0;
}


