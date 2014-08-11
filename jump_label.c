/*
Original code copyright:

Copyright (C) 2009-2012 Jason Baron <jbaron@redhat.com>
Copyright (C) 2011-2012 Peter Zijlstra <pzijlstr@redhat.com>

New code / modified code copyright:

Copyright (C) 2014 Patrick McCormick <patm@pdx.edu>

Distributable under the terms of the GPLv2, see LICENSE for full text.
*/
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <s-keys.h>
#include <misc.h>


// need to translate to c11 atomic ops or not? stub for now

static inline int atomic_read(const atomic_t *v)
{
	return (*(volatile int *)&(v)->counter);
}

int atomic_inc(atomic_t *p)
{
	return ++(p->counter);
}

/**
 * Increment by 1 if p is non-zero, otherwise do nothing.
 * Return non-zero if p was incremented.
**/
int atomic_inc_not_zero(atomic_t *p)
{
	if (p->counter) {
		p->counter += 1;
		return p->counter;
	} else
		return 0;
}

/**
 * Decrement, no mutex for now
 */
int atomic_dec_and_mutex_lock(atomic_t *p)
{
	return --(p->counter);
}

// stub out kernel_text_address() for now
int kernel_text_address(unsigned long addr)
{
	return 1;
}

static int jump_label_cmp(const void *a, const void *b)
{
	const struct jump_entry *jea = a;
	const struct jump_entry *jeb = b;

	if (jea->key < jeb->key)
		return -1;

	if (jea->key > jeb->key)
		return 1;

	return 0;
}

static void
jump_label_sort_entries(struct jump_entry *start, struct jump_entry *stop)
{
	unsigned long size;

	size = (((unsigned long)stop - (unsigned long)start)
					/ sizeof(struct jump_entry));
	qsort(start, size, sizeof(struct jump_entry), jump_label_cmp);
}

static void jump_label_update(struct static_key *key, int enable);

/* export */
void static_key_slow_inc(struct static_key *key)
{
	STATIC_KEY_CHECK_USE();
	if (atomic_inc_not_zero(&key->enabled))
		return;

    // jl lock()	
	if (atomic_read(&key->enabled) == 0) {
		if (!jump_label_get_branch_default(key))
			jump_label_update(key, JUMP_LABEL_ENABLE);
		else
			jump_label_update(key, JUMP_LABEL_DISABLE);
	}
	atomic_inc(&key->enabled);
	// jl unlock()
}

static void __static_key_slow_dec(struct static_key *key)
{
	// lock and aquire mutex
	if (!atomic_dec_and_mutex_lock(&key->enabled)) {
		WARN(atomic_read(&key->enabled) < 0,
		     "jump label: negative count!\n");
		return;
	}

	if (!jump_label_get_branch_default(key))
		jump_label_update(key, JUMP_LABEL_DISABLE);
	else
		jump_label_update(key, JUMP_LABEL_ENABLE);
//	unlock mutex()
}

/* export */
void static_key_slow_dec(struct static_key *key)
{
	STATIC_KEY_CHECK_USE();
	__static_key_slow_dec(key);
}

static int addr_conflict(struct jump_entry *entry, void *start, void *end)
{
	if (entry->code <= (unsigned long)end &&
		entry->code + JUMP_LABEL_NOP_SIZE > (unsigned long)start)
		return 1;

	return 0;
}

static int __jump_label_text_reserved(struct jump_entry *iter_start,
		struct jump_entry *iter_stop, void *start, void *end)
{
	struct jump_entry *iter;

	iter = iter_start;
	while (iter < iter_stop) {
		if (addr_conflict(iter, start, end))
			return 1;
		iter++;
	}

	return 0;
}

/* 
 * Update code which is definitely not currently executing.
 * Architectures which need heavyweight synchronization to modify
 * running code can override this to make the non-live update case
 * cheaper.
 */
void arch_jump_label_transform_static(struct jump_entry *entry,
					    enum jump_label_type type)
{
	arch_jump_label_transform(entry, type);	
}

static void __jump_label_update(struct static_key *key,
				struct jump_entry *entry,
				struct jump_entry *stop, int enable)
{
	for (; (entry < stop) &&
	      (entry->key == (jump_label_t)(unsigned long)key);
	      entry++) {
		/*
		 * entry->code set to 0 invalidates module init text sections
		 * kernel_text_address() verifies we are not in core kernel
		 * init code, see jump_label_invalidate_module_init().
		 */
		if (entry->code && kernel_text_address(entry->code))
			arch_jump_label_transform(entry, enable);
	}
}

static enum jump_label_type jump_label_type(struct static_key *key)
{
	bool true_branch = jump_label_get_branch_default(key);
	bool state = static_key_enabled(key);

	if ((!true_branch && state) || (true_branch && !state))
		return JUMP_LABEL_ENABLE;

	return JUMP_LABEL_DISABLE;
}


void jump_label_init(void)
{
	struct jump_entry *iter_start = __start___jump_table;
	struct jump_entry *iter_stop = __stop___jump_table;
	struct static_key *key = NULL;
	struct jump_entry *iter;

	// jl lock()
	jump_label_sort_entries(iter_start, iter_stop);

	for (iter = iter_start; iter < iter_stop; iter++) {
		struct static_key *iterk;

		iterk = (struct static_key *)(unsigned long)iter->key;
		arch_jump_label_transform_static(iter, jump_label_type(iterk));
		if (iterk == key)
			continue;

		key = iterk;
		/*
		 * Set key->entries to iter, but preserve JUMP_LABEL_TRUE_BRANCH.
		 */
		*((unsigned long *)&key->entries) += (unsigned long)iter;
	}
	static_key_initialized = true;
	// jl unlock
}

/***
 * jump_label_text_reserved - check if addr range is reserved
 * @start: start text addr
 * @end: end text addr
 *
 * checks if the text addr located between @start and @end
 * overlaps with any of the jump label patch addresses. Code
 * that wants to modify kernel text should first verify that
 * it does not overlap with any of the jump label addresses.
 * Caller must hold jump_label_mutex.
 *
 * returns 1 if there is an overlap, 0 otherwise
 */
int jump_label_text_reserved(void *start, void *end)
{
	int ret = __jump_label_text_reserved(__start___jump_table,
			__stop___jump_table, start, end);

	if (ret)
		return ret;

	return ret;
}

static void jump_label_update(struct static_key *key, int enable)
{
	struct jump_entry *stop = __stop___jump_table;
	struct jump_entry *entry = jump_label_get_entries(key);

	/* if there are no users, entry can be NULL */
	if (entry)
		__jump_label_update(key, entry, stop, enable);
}

