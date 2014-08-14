#include <stdio.h>

#include <s-keys.h>

struct static_key key = STATIC_KEY_INIT_FALSE;

int main (int argc, char *argv[])
{
	jump_label_init();



	if (static_key_false(&key))
		printf("...\n");
	else
		printf("%d\n", argc);

	static_key_slow_inc(&key);

	if (static_key_false(&key))
		printf("...\n");
	else
		printf("%d\n", argc);

	static_key_slow_dec(&key);
	static_key_slow_dec(&key);

	if (static_key_false(&key))
		printf("...\n");
	else
		printf("%d\n", argc);

	return 0;
}


