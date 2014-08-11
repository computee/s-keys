#ifndef _X86_JUMP_LABEL_H
#define _X86_JUMP_LABEL_H

#include <stdint.h>
#include <stdbool.h>

#include <nops.h>
#include <asm.h>
#include <misc.h>

#define JUMP_LABEL_NOP_SIZE 5

#ifdef __LP64__
# define STATIC_KEY_INIT_NOP P6_NOP5_ATOMIC
#else
# define STATIC_KEY_INIT_NOP GENERIC_NOP5_ATOMIC
#endif

#ifdef __LP64__
typedef uint64_t jump_label_t;
#else
typedef uint32_t jump_label_t;
#endif

struct static_key;

struct jump_entry {
	jump_label_t code;
	jump_label_t target;
	jump_label_t key;
};

static bool __always_inline arch_static_branch(struct static_key *key)
{
	asm_volatile_goto ("1:"
		".byte " stringify(STATIC_KEY_INIT_NOP) "\n\t"
		".pushsection __jump_table,  \"aw\" \n\t"
		_ASM_ALIGN "\n\t"
		_ASM_PTR "1b, %l[l_yes], %c0 \n\t"
		".popsection \n\t"
		: : "i" (key) : : l_yes);

	return false;
l_yes:
	return true;
}

#endif
