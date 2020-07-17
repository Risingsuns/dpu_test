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
void    *mapped_reg_base;
void    *mapped_ddr_base;
void    *mapped_inst_base;
void    *mapped_weight_base;

char    data_path[]     = "./param/vggnet_512_inst/ddr.imgs.in.rtl.bin";
char    instr_path[]    = "./param/vggnet_512_inst/inst.rtl.bin";
char    weights_path[]  = "./param/vggnet_512_inst/ddr.weights.rtl.bin";
char    out_path[]      = "./param/vggnet_512_inst/outdata.bin";
char    golden_path[]   = "./param/vggnet_512_inst/ddr.imgs.out.rtl.bin";

#define OUT_DATA_OFFSET 0x24c00

#undef readl
#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#undef writel
#define writel(addr,b) (void)((*(volatile unsigned int *) (addr)) = (b))

#define REG_BASE_ADDRESS        0x80000000
#define DDR_BASE_ADDRESS        0x46000000
#define INST_BASE_ADDRESS       0x33000000
#define WEIGHT_BASE_ADDRESS     0x36000000

void *memory_map(unsigned int map_size, off_t base_addr) //map_size = n MByte
{
    void *mapped_base;
    mapped_base = mmap(0, map_size*1024*1024, PROT_READ | PROT_WRITE, MAP_SHARED
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


int load_bin(char *path, void* offset)
{
    int pb_in=open(path, O_RDONLY);
    long size;
    int numread=0;

    if(pb_in == -1){
        printf("Err: open %s: %s\n",__func__, path);
        return -1;
    }

    // obtain file size:
    struct stat filestate;
    stat(path, &filestate);
    size = filestate.st_size;

    if ((numread = read(pb_in, offset, size)) != size){
        printf("Err: fread error, actually read: %d\n",numread);
    }
    close(pb_in);
    return 0;
}

static void timespec_sub(struct timespec *t1, const struct timespec *t2)
{
  assert(t1->tv_nsec >= 0);
  assert(t1->tv_nsec < 1000000000);
  assert(t2->tv_nsec >= 0);
  assert(t2->tv_nsec < 1000000000);
  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;
  if (t1->tv_nsec >= 1000000000)
  {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000;
  }
  else if (t1->tv_nsec < 0)
  {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000;
  }
}

int dump(char *path, void* from) // legacy
{
    FILE *pb_out, *pb_golden;
    long size;

    pb_out=fopen(path,"wb");

    //obtain golden file size
    struct stat filestate;
    stat(golden_path, &filestate);
    size = filestate.st_size;

    if(pb_out==NULL){
        printf("dump_ddr:open file error\n");
        return 1;
    }
    
    fwrite(from, 1, size, pb_out);

    fclose(pb_out);
    return 0;
}

int init_fpga(){

    printf("start loading bin file\n");
    load_bin(data_path,    mapped_ddr_base);
    load_bin(weights_path, mapped_weight_base);
    load_bin(instr_path,   mapped_inst_base);
    printf("finish loading bin file\n");

#ifdef DEBUG
    unsigned int instr_data;
    int i = 0;
    for(i=0;i<10;i++){ 
        instr_data = readl(mapped_ddr_base+4*i);
        printf("%08x\n", instr_data);
    }
    for(i=0;i<10;i++){ 
        instr_data = readl(mapped_weight_base+4*i);
        printf("%08x\n", instr_data);
    }
    for(i=0;i<10;i++){ 
        instr_data = readl(mapped_inst_base+4*i);
        printf("%08x\n", instr_data);
    }
#endif   
    ////// RUN !!!!/////
    printf("start configuring dpu registers\n");
    writel(mapped_reg_base+0x08,INST_BASE_ADDRESS);
    writel(mapped_reg_base+0x0c,DDR_BASE_ADDRESS);
    writel(mapped_reg_base+0x10,WEIGHT_BASE_ADDRESS);
    writel(mapped_reg_base+0x14,0);
    usleep(1000);
    writel(mapped_reg_base+0x14,1);
    printf("finish configuring dpu registers\n");
    ////// WAIT //////
	return 0;
}

int main(){
  off_t   reg_base = REG_BASE_ADDRESS;
  off_t   ddr_base = DDR_BASE_ADDRESS;
  off_t   inst_base = INST_BASE_ADDRESS;    
  off_t   weight_base = WEIGHT_BASE_ADDRESS;
    
  struct timespec ts_start, ts_end;
  int rc;

  printf("start mapping memory to user space\n");
  memfd = open("/dev/mem", O_RDWR | O_SYNC);
  mapped_reg_base = memory_map(1,reg_base);
  mapped_ddr_base = memory_map(128,ddr_base);
  mapped_inst_base = memory_map(16,inst_base);
  mapped_weight_base = memory_map(64, weight_base);
  printf("finish mapping memory!\n\n");

  printf("start initing dpu\n");
  init_fpga();
  printf("finish initing dpu\n\n");

  
   rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
   while (!(readl(mapped_reg_base + 0x14) & 0x2)){
        usleep(1);
   }
   rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);

   timespec_sub(&ts_end, &ts_start);
   printf("CLOCK_MONOTONIC reports %ld.%09ld seconds\n\n",
     ts_end.tv_sec, ts_end.tv_nsec);
  
  printf("start copying result\n");
  dump(out_path,mapped_ddr_base+OUT_DATA_OFFSET);
  printf("finish copying result\n\n");

  printf("start unmapping memory\n");
  memory_unmap(1,mapped_reg_base);
  memory_unmap(128,mapped_ddr_base);
  memory_unmap(16,mapped_inst_base);
  memory_unmap(64, mapped_weight_base);
  printf("finish unmapping memory\n");
  
  return 0;
}
