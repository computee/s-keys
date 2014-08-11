/*
Original code copyright:

Copyright (C) 2009-2012 Jason Baron <jbaron@redhat.com>
Copyright (C) 2011-2012 Peter Zijlstra <pzijlstr@redhat.com>

New code / modified code copyright:

Copyright (C) 2014 Patrick McCormick <patm@pdx.edu>

Distributable under the terms of the GPLv2, see LICENSE for full text.
*/
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* check for gcc >= 4.8.2, on anything else
 * fall back to normal branching code */
#ifdef __GNUC__

    #if __GNUC__ > 4 || \
	(__GNUC__ == 4 && (__GNUC_MINOR__ > 8 || \
	(__GNUC_MINOR__ == 8 && \
	__GNUC_PATCHLEVEL__ >= 2)))

	#define ENABLE_SK

    #else
	#warn "Disabling s-keys, non gcc or gcc < 4.8.2"
    #endif

#endif

/* architecture plus size detection */
#ifdef ENABLE_SK

    #if defined(__x86_64__) || defined(__i386__)
		#include <s-keys_x86.h>
    #elif defined(__arm__)
		#include <s-keys_arm.h>
    #else
	#error "Unknown architecture"
    #endif

#endif

// atomic defs should go in arch-dependent header:
typedef struct {
	int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }

#define WARN(x,...) if (x) fprintf(stderr, __VA_ARGS__);

static bool static_key_initialized = false;

#define STATIC_KEY_CHECK_USE() WARN(!static_key_initialized,		      \
				    "%s used before call to jump_label_init", \
				    __func__)


struct static_key {
	atomic_t enabled;
/* Set lsb bit to 1 if branch is default true, 0 ot */
	struct jump_entry *entries;
};

enum jump_label_type {
	JUMP_LABEL_DISABLE = 0,
	JUMP_LABEL_ENABLE,
};

static inline atomic_t static_key_count(struct static_key *key)
{
	//return atomic_read(&key->enabled);
	return key->enabled;
}

#ifdef ENABLE_SK

#define JUMP_LABEL_TYPE_FALSE_BRANCH	0UL
#define JUMP_LABEL_TYPE_TRUE_BRANCH	1UL
#define JUMP_LABEL_TYPE_MASK		1UL

static
inline struct jump_entry *jump_label_get_entries(struct static_key *key)
{
	return (struct jump_entry *)((unsigned long)key->entries
						& ~JUMP_LABEL_TYPE_MASK);
}

static inline bool jump_label_get_branch_default(struct static_key *key)
{
	if (((unsigned long)key->entries & JUMP_LABEL_TYPE_MASK) ==
	    JUMP_LABEL_TYPE_TRUE_BRANCH)
		return true;
	return false;
}

static __always_inline bool static_key_false(struct static_key *key)
{
	return arch_static_branch(key);
}

static __always_inline bool static_key_true(struct static_key *key)
{
	return !static_key_false(key);
}

// platform specific functions/defines needed:

// currently in the x86 header/impl:
// bool arch_static_branch(struct static_key *key)
// struct jump_entry {

extern struct jump_entry __start___jump_table[] __attribute__ ((section ("__jump_table")));
extern struct jump_entry __stop___jump_table[] __attribute__ ((section ("__jump_table")));

void arch_jump_label_transform(struct jump_entry *entry, enum jump_label_type type);

// in generic jl impl:
void jump_label_init(void);
void jump_label_lock(void);
void jump_label_unlock(void);

int jump_label_text_reserved(void *start, void *end);

void static_key_slow_inc(struct static_key *key);
void static_key_slow_dec(struct static_key *key);

// both !?
void arch_jump_label_transform_static(struct jump_entry *entry, enum jump_label_type type);


//extern void jump_label_apply_nops(struct module *mod);

#define STATIC_KEY_INIT_TRUE 		\
	{ .enabled = ATOMIC_INIT(1),				\
	  .entries = (void *)JUMP_LABEL_TYPE_TRUE_BRANCH }
#define STATIC_KEY_INIT_FALSE		\
	{ .enabled = ATOMIC_INIT(0),				\
	  .entries = (void *)JUMP_LABEL_TYPE_FALSE_BRANCH }

#else  /* !ENABLE_SK */

static __always_inline void jump_label_init(void)
{
	static_key_initialized = true;
}

static __always_inline bool static_key_false(struct static_key *key)
{
	if (unlikely(static_key_count(key) > 0))
		return true;
	return false;
}

static __always_inline bool static_key_true(struct static_key *key)
{
	if (likely(static_key_count(key) > 0))
		return true;
	return false;
}

static inline void static_key_slow_inc(struct static_key *key)
{
	STATIC_KEY_CHECK_USE();
	atomic_inc(&key->enabled);
}

static inline void static_key_slow_dec(struct static_key *key)
{
	STATIC_KEY_CHECK_USE();
	atomic_dec(&key->enabled);
}

static inline int jump_label_text_reserved(void *start, void *end)
{
	return 0;
}

#define STATIC_KEY_INIT_TRUE ((struct static_key) \
		{ .enabled = ATOMIC_INIT(1) })
#define STATIC_KEY_INIT_FALSE ((struct static_key) \
		{ .enabled = ATOMIC_INIT(0) })

#endif	/* HAVE_JUMP_LABEL */

#define STATIC_KEY_INIT STATIC_KEY_INIT_FALSE

static inline bool static_key_enabled(struct static_key *key)
{
	return static_key_count(key).counter > 0;
}

