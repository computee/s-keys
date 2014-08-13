#pragma once

#define __ASM_FORM(x)	" " #x " "
#define __ASM_FORM_RAW(x)     #x
#define __ASM_FORM_COMMA(x) " " #x ","

#ifndef __LP64__
# define __ASM_SEL(a,b) __ASM_FORM(a)
# define __ASM_SEL_RAW(a,b) __ASM_FORM_RAW(a)
#else
# define __ASM_SEL(a,b) __ASM_FORM(b)
# define __ASM_SEL_RAW(a,b) __ASM_FORM_RAW(b)
#endif

#define _ASM_PTR	__ASM_SEL(.long, .quad)
#define _ASM_ALIGN	__ASM_SEL(.balign 4, .balign 8)

