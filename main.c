#include <stdio.h>

#include <jump_label.h>

static struct static_key key = STATIC_KEY_INIT_FALSE;

int main (int argc, char *argv[])
{
	jump_label_init();
//	arch_static_branch(&key);


	static_key_slow_inc(&key);


	if (static_key_false(&key))
		printf("true\n");
	else
		printf("false\n");

	return 0;
}


