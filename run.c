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
#include <math.h>
//#include <opencv2/core.hpp>
//#include <opencv2/highgui.hpp>
//#include <opencv2/imgproc.hpp>

int     memfd;
void    *mapped_reg_base;
void    *mapped_ddr_base;
void    *mapped_inst_base;
void    *mapped_weight_base;

char    data_path[]     = "./param/vo/";
char    instr_path[]    = "./param/vo/inst.rtl.bin";
char    weights_path[]  = "./param/vo/ddr.weights.rtl.bin";
//char    out_path[]      = "./param/vo/outdata.bin";
//char    golden_path[]   = "./param/vo/ddr.imgs.out.rtl.bin";

#define IN_DATA_SIZE 1277952
#define OUT_DATA_OFFSET 2945536
#define OUT_DATA_SIZE 84

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
/*
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
*/
int init_fpga(){

    printf("start loading bin file\n");
    //load_bin(data_path,    mapped_ddr_base);
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
    //writel(mapped_reg_base+0x14,0);
    //usleep(1000);
    //writel(mapped_reg_base+0x14,1);
    printf("finish configuring dpu registers\n");
    ////// WAIT //////
	return 0;
}

int run(){
    writel(mapped_reg_base+0x14,0);
    usleep(1000);
    writel(mapped_reg_base+0x14,1);
    return 0;
}
/*
void dpuSetInputImage2(const char *img_path1, const char *img_path2, int8_t *img_addr, int height, int width) {
    cv::Mat img1 = cv::imread(img_path1), img_res1;
    img_res1 = cv::Mat(height, width, CV_8SC3);
    //if(img1.rows != width){
    //    cv::resize(img1, img_res1, cv::Size(height, width),0,0,cv::INTER_LINEAR);
    //}else{
        img_res1 = img1;
    //}
    
    cv::Mat img2 = cv::imread(img_path2), img_res2;
    img_res2 = cv::Mat(height, width, CV_8SC3);
    //if(img2.rows != width){
    //    cv::resize(img2, img_res2, cv::Size(height, width),0,0,cv::INTER_LINEAR);
    //}else{
        img_res2 = img2;
    //}
    
    for(int i_w = 0; i_w<3; i_w++){
      for(int i = 0; i<3; i++){
        cout << "ori img =" << (float)img.at<Vec3b>(0,i_w)[i] << endl;
        cout << "resize img = " << (float)img_res.at<Vec3b>(0,i_w)[i] << endl;
      }
    }
    for(int i = 0; i<3; i++){
      cout << "1314 resize img = " << (float)img_res.at<Vec3b>(438, 0)[i] << endl;
    }
    
    int8_t *addr = img_addr;
    
    float scale = 0.00390625f;
    int value;
    float mean[3] = {128, 128, 128};
    float temp;
    int count = 0;
    for(int i_h = 0; i_h < img_res1.rows; i_h++){
        for(int i_w = 0; i_w < img_res1.cols; i_w++){
            for(int i_c = 0; i_c < 3 ; i_c++){
                temp = ((float)img_res1.at<cv::Vec3b>(i_h,i_w)[2-i_c] - mean[2-i_c])*scale;
                value = (int)round(temp*128);
                *(addr++) = (int8_t)value;
            }
            for(int i_c = 0; i_c < 3 ; i_c++){
                temp = ((float)img_res2.at<cv::Vec3b>(i_h,i_w)[2-i_c] - mean[2-i_c])*scale;
                value = (int)round(temp*128);
                *(addr++) = (int8_t)value;
            }
        }
    }
}
*/
void dpuGetResultFrom(int8_t *to, int8_t *from, int size){
    for(int i = 0; i < size; ++i){
        to[i] = *(from+i);
    }
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

  int num = 1591;
  char file[50];
  int8_t output[OUT_DATA_SIZE];
  FILE *outfile = fopen("out_data.txt", "w");
  for(int i = 1; i < num; ++i){
    sprintf(file, "dataset/bin/%d.bin", i);
    //dpuSetInputImage2(img1, img2, (int8_t*)mapped_ddr_base, 256, 832);
    load_bin(file, mapped_ddr_base);
    run();
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    while (!(readl(mapped_reg_base + 0x14) & 0x2)){
        usleep(1);
    }
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);

    timespec_sub(&ts_end, &ts_start);
    printf("CLOCK_MONOTONIC reports %ld.%09ld seconds\n\n", ts_end.tv_sec, ts_end.tv_nsec);
  
    printf("start copying result\n");
    dpuGetResultFrom(output, (int8_t*)mapped_ddr_base+OUT_DATA_OFFSET, OUT_DATA_SIZE);
    double result[6] = {0,0,0,0,0,0};
    for(int i = 0; i < OUT_DATA_SIZE; ++i){
        result[i%6] += 0.01*(float)output[i] / 16 / (OUT_DATA_SIZE/6);
        //printf("%d\n", output[i]);
    }
    for(int i = 0; i < 6; ++i){
        fprintf(outfile, "%f ", result[i]);
    }
    fprintf(outfile, "\n");
    //dump(out_path,mapped_ddr_base+OUT_DATA_OFFSET);
    printf("finish copying result\n\n");
  }
  fclose(outfile);
  printf("start unmapping memory\n");
  memory_unmap(1,mapped_reg_base);
  memory_unmap(128,mapped_ddr_base);
  memory_unmap(16,mapped_inst_base);
  memory_unmap(64, mapped_weight_base);
  printf("finish unmapping memory\n");
  
  return 0;
}
