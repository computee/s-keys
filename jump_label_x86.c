/*
 * jump label x86 support
 *
 * Copyright (C) 2009 Jason Baron <jbaron@redhat.com>
 *
 */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

#include <jump_label.h>
#include <jump_label_x86.h>

static inline void *pageof(const void* p)
{
	size_t pagesize = sysconf(_SC_PAGESIZE);
	return (void*) ((uintptr_t)p & ~(pagesize - 1));
}

// text_poke are kernel equivalents of mprotect, stub out for now:

void *sk_mprotect(void *addr, size_t len)
{
	printf("%p\n", addr);
	printf("%p\n", pageof(addr));

	if (mprotect(pageof(addr), len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
		perror("mprotect");
		return NULL;
	}

	return addr;
}

union jump_code_union {
	char code[JUMP_LABEL_NOP_SIZE];
	struct {
		char jump;
		int offset;
	} __attribute__((packed));
};

static void bug_at(unsigned char *ip, int line)
{
	/*
	 * The location is not an op that we were expecting.
	 * Something went wrong. Crash the box, as something could be
	 * corrupting the kernel.
	 */

	printf("Unexpected op at %pS [%p] (%02x %02x %02x %02x %02x) %s:%d\n",
	       ip, ip, ip[0], ip[1], ip[2], ip[3], ip[4], __FILE__, line);
	assert(false);
}

static void __jump_label_transform(struct jump_entry *entry,
				   enum jump_label_type type,
				   int init)
{
	union jump_code_union code;
	const unsigned char default_nop[] = { STATIC_KEY_INIT_NOP };
	const unsigned char ideal_nop[] = { STATIC_KEY_INIT_NOP };

	if (type == JUMP_LABEL_ENABLE) {
		if (init) {
			/*
			 * Jump label is enabled for the first time.
			 * So we expect a default_nop...
			 */
			if (unlikely(memcmp((void *)entry->code, default_nop, 5)
				     != 0))
				bug_at((void *)entry->code, __LINE__);
		} else {
			/*
			 * ...otherwise expect an ideal_nop. Otherwise
			 * something went horribly wrong.
			 */
			if (unlikely(memcmp((void *)entry->code, ideal_nop, 5) != 0))
				bug_at((void *)entry->code, __LINE__);
		}

		code.jump = 0xe9;
		code.offset = entry->target -
				(entry->code + JUMP_LABEL_NOP_SIZE);
	} else {
		/*
		 * We are disabling this jump label. If it is not what
		 * we think it is, then something must have gone wrong.
		 * If this is the first initialization call, then we
		 * are converting the default nop to the ideal nop.
		 */
		if (init) {
			if (unlikely(memcmp((void *)entry->code, default_nop, 5) != 0))
				bug_at((void *)entry->code, __LINE__);
		} else {
			code.jump = 0xe9;
			code.offset = entry->target -
				(entry->code + JUMP_LABEL_NOP_SIZE);
			if (unlikely(memcmp((void *)entry->code, &code, 5) != 0))
				bug_at((void *)entry->code, __LINE__);
		}
		memcpy(&code, ideal_nop, JUMP_LABEL_NOP_SIZE);
	}

	/*
	 * Make text_poke_bp() a default fallback poker.
	 *
	 * At the time the change is being done, just ignore whether we
	 * are doing nop -> jump or jump -> nop transition, and assume
	 * always nop being the 'currently valid' instruction
	 *
	 */

	if (sk_mprotect((void *)entry->code, JUMP_LABEL_NOP_SIZE) == NULL)
	    bug_at("sk_mprotect", __LINE__);
	
	// and now do the actual copy:
	memcpy((void*) entry->code, &code, JUMP_LABEL_NOP_SIZE);
	inits = 0;
}

void arch_jump_label_transform(struct jump_entry *entry,
			       enum jump_label_type type)
{
	__jump_label_transform(entry, type, 0);
}

