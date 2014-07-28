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

void *text_poke_bp(void *addr, const void *opcode, size_t len, void *handler)
{
	printf("%i\n", addr);
	printf("%i\n", pageof(addr));

	if (mprotect(pageof(addr), len, PROT_READ | PROT_WRITE | PROT_EXEC))
		perror("mprotect");

	return addr;
}

void *text_poke_early(void *addr, const void *opcode, size_t len)
{
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

/*
	pr_warning("Unexpected op at %pS [%p] (%02x %02x %02x %02x %02x) %s:%d\n",
	       ip, ip, ip[0], ip[1], ip[2], ip[3], ip[4], __FILE__, line);
	BUG();
*/

	assert(false);
}

static void __jump_label_transform(struct jump_entry *entry,
				   enum jump_label_type type,
				   void *(*poker)(void *, const void *, size_t),
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
			if (unlikely(memcmp((void *)entry->code, ideal_nop, 5)
				     != 0))
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
		memcpy(&code, ideal_nops[NOP_ATOMIC5], JUMP_LABEL_NOP_SIZE);
	}

	/*
	 * Make text_poke_bp() a default fallback poker.
	 *
	 * At the time the change is being done, just ignore whether we
	 * are doing nop -> jump or jump -> nop transition, and assume
	 * always nop being the 'currently valid' instruction
	 *
	 */
	if (poker)
		(*poker)((void *)entry->code, &code, JUMP_LABEL_NOP_SIZE);
	else
		text_poke_bp((void *)entry->code, &code, JUMP_LABEL_NOP_SIZE,
			     (void *)entry->code + JUMP_LABEL_NOP_SIZE);
}

void arch_jump_label_transform(struct jump_entry *entry,
			       enum jump_label_type type)
{
	__jump_label_transform(entry, type, NULL, 0);
}

static enum {
	JL_STATE_START,
	JL_STATE_NO_UPDATE,
	JL_STATE_UPDATE,
} jlstate = JL_STATE_START;

#ifdef CONFIG_MODULES

void arch_jump_label_transform_static(struct jump_entry *entry,
				      enum jump_label_type type)
{
	/*
	 * This function is called at boot up and when modules are
	 * first loaded. Check if the default nop, the one that is
	 * inserted at compile time, is the ideal nop. If it is, then
	 * we do not need to update the nop, and we can leave it as is.
	 * If it is not, then we need to update the nop to the ideal nop.
	 */
	if (jlstate == JL_STATE_START) {
		const unsigned char default_nop[] = { STATIC_KEY_INIT_NOP };
		const unsigned char ideal_nop[] = { STATIC_KEY_INIT_NOP };

		if (memcmp(ideal_nop, default_nop, 5) != 0)
			jlstate = JL_STATE_UPDATE;
		else
			jlstate = JL_STATE_NO_UPDATE;
	}
	if (jlstate == JL_STATE_UPDATE)
		__jump_label_transform(entry, type, text_poke_early, 1);
}

#endif 
