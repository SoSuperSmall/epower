#ifndef __EPSHMEM_H
#define __EPSHMEM_H
#define SHARED_MEM_BASE 0x60000000
extern void *alloc_shared_mem(int key, void *addr_start, unsigned int memsize);
extern void *allocated_shared_mem(int key, void *addr_start, unsigned int memsize);
extern int free_shared_mem(void *shmaddr);
#endif
