#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

int     memfd;
void    *mapped_reg_base_0;
void    *mapped_reg_base_1;

#undef readl
#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#undef writel
#define writel(addr,b) (void)((*(volatile unsigned int *) (addr)) = (b))

#define REG_BASE_ADDRESS_0      0x80000000
#define REG_BASE_ADDRESS_1      0x80001000

void *memory_map(unsigned int map_size, off_t base_addr) //map_size = n KByte
{
    void *mapped_base;
    mapped_base = mmap(0, map_size*1024, PROT_READ | PROT_WRITE, MAP_SHARED
, memfd, base_addr);
    if (mapped_base == (void *) -1) {
        printf("Can't map memory to user space.\n");
        exit(0);
    }
#ifdef DEBUG
    printf("Memory mapped at address %p.\n", mapped_base);
#endif
    return mapped_base;
}


void memory_unmap(unsigned int map_size, void *mapped_base)
{
    if (munmap(mapped_base, map_size*1024*1024) == -1) {
        printf("Can't unmap memory from user space.\n");
        exit(0);
    }
}

int main(){
  off_t   reg_base_0 = REG_BASE_ADDRESS_0;
  off_t   reg_base_1 = REG_BASE_ADDRESS_1;

  unsigned int instr_data;

  printf("start mapping memory to user space\n");
  memfd = open("/dev/mem", O_RDWR | O_SYNC);
  mapped_reg_base_0 = memory_map(4,reg_base_0);
  mapped_reg_base_1 = memory_map(4,reg_base_1);
  printf("finish mapping memory!\n\n");

  instr_data = readl(mapped_reg_base_0);
  printf("%08x\n", instr_data);
  instr_data = readl(mapped_reg_base_0+4);
  printf("%08x\n", instr_data);
  instr_data = readl(mapped_reg_base_1);
  printf("%08x\n", instr_data);
  instr_data = readl(mapped_reg_base_1+4);
  printf("%08x\n", instr_data);
  
  return 0;
}
