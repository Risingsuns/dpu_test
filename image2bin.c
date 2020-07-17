#include "stdio.h"
#include "stdlib.h"

int main(){
    
    int8_t a[6*370*1226];
    for(int i = 0; i < 6*370*1226; ++i) a[i] = 127;
    FILE *fid;
    fid = fopen("param/test/data.bin", "wb");
    fwrite(a, sizeof(int8_t), 6*370*1226, fid);
    fclose(fid);
    
    return 0;
}
