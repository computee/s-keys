#include <stdio.h>

#include <s-keys.h>

struct static_key key = STATIC_KEY_INIT_FALSE;

int main (int argc, char *argv[])
{
    int ret = 0;
	jump_label_init();

	if (static_key_false(&key))
		++ret;
	else
		ret += 0;

	static_key_slow_inc(&key);

	if (static_key_false(&key))
		ret += 0;
	else
		++ret;

	if (ret)
		printf("%s\t\tFAIL\n", __FILE__);
	else
		printf("%s\t\tOK\n", __FILE__);

	return 0;
}


