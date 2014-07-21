#include <stdint.h>

#define cmpxchg( ptr, _old, _new ) { \
  volatile uint32_t *__ptr = (volatile uint32_t *)(ptr);   \
  uint32_t __ret;                                     \
  __asm__ volatile( "lock; cmpxchgl %2,%1"           \
    : "=a" (__ret), "+m" (*__ptr)                \
    : "r" (_new), "0" (_old)                     \
    : "memory");				 \
  __ret;                                         \
}

int main (int argc, char * argv[])
{

struct Item {
  volatile struct Item* next;
};
 
volatile struct Item *head;
 
void addItem( struct Item *i ) {
  volatile struct Item *oldHead;

again:
  oldHead = head;
  i->next = oldHead;
  cmpxchg( &head, oldHead, i);
}

	return 0;
}
