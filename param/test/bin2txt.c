#include "stdio.h"
#include "stdlib.h"

#define SIZE 32

int main(){
    char binfile[] = "outdata.bin";
    char txtfile[] = "outdata.txt";

    FILE *fin = fopen(binfile, "rb");
    
    int8_t a[SIZE];
    fread((void*)a, 1, SIZE, fin);
    //for(int i = 0; i < SIZE; ++i)
    //   printf("%d\n", a[i]);
    fclose(fin);
    
    FILE *fout = fopen(txtfile, "w");
    for(int i = 0; i < SIZE; ++i)
        fprintf(fout, "%d\n", a[i]);
    fclose(fout);
    return 0;
}
