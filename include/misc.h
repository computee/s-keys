#ifndef _MISC_H
#define _MISC_H

/* misc macros needed for compilation */

#define __stringify(x...)       #x 
#define stringify(x...)         __stringify(x)

#define asm_volatile_goto(x) do { __asm__ goto (x); __asm__ (""); } while (0)
 
#define likely(x)      __builtin_expect(!!(x), 1) 
#define unlikely(x)    __builtin_expect(!!(x), 0) 

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))

 
#endif /* _MISC_H */
