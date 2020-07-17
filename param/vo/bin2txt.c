#include "stdio.h"
#include "stdlib.h"

#define SIZE 192

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
    for(int c = 0; c < 6; ++c){
        for(int r = 0; r < 3; ++r){
            for(int l = 0; l < 10; ++l)
                printf("%.4f ", (double)a[10*6*r+6*l+c]/16);
            printf("\n");
        }
        printf("\n");
    }
    return 0;
}
