#include <stdio.h>

#include <s-keys.h>

struct static_key key = STATIC_KEY_INIT_TRUE;

int main (int argc, char *argv[])
{
	jump_label_init();

	if (static_key_true(&key)) {
		printf("%s\t\tOK\n", __FILE__);
		return 0;
	}
	else {
		printf("%s\t\tFAIL\n", __FILE__);
		return 1;
	}

}
