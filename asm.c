#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <nops.h>
#include <misc.h>
#include <asm.h>

#define STATIC_KEY_INIT_NOP P6_NOP5_ATOMIC

struct jump_entry {
        uint64_t code;
        uint64_t target;
        uint64_t key;
};

typedef struct {
        int counter;
} atomic_t;


struct static_key {
        atomic_t enabled;
        struct jump_entry *entries;
};

struct static_key key;

struct jump_entry __start___jump_table[] __attribute__ ((section ("__jump_table")));
struct jump_entry __stop___jump_table[] __attribute__ ((section ("__jump_table")));

static bool __always_inline f(struct static_key *k)
{
	__asm__ volatile goto ("1:"
		".byte " stringify(STATIC_KEY_INIT_NOP) "\n\t"
		".pushsection __jump_table,  \"aw\" \n\t"
		_ASM_ALIGN "\n\t"
		_ASM_PTR "1b, %l[l_yes], %c0 \n\t"
		".popsection \n\t"
		: : "i" (k) : : l_yes);
	return false;
l_yes:
	return true;
}

int main (int argc, char *argv[])
{
//    __asm__ volatile goto (_ASM_ALIGN "\n\t" _ASM_PTR  "%l[l_yes]\n\t" : : : : l_yes );
    __asm__ volatile (".pushsection __jump_table,  \"aw\" \n\t"
			".byte " stringify(STATIC_KEY_INIT_NOP) "\n\t"
			".byte " stringify(STATIC_KEY_INIT_NOP) "\n\t"
			".byte " stringify(STATIC_KEY_INIT_NOP) "\n\t"
			".popsection \n\t" );

    return 0;
}


