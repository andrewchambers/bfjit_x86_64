#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include <sys/mman.h>
#include <unistd.h>

#define ASM_SIZE 1000000

int main(int c, char * argv []) {
    
    if(c < 2){
        fprintf(stderr,"usage: %s prog.b [asm.bin]\n",argv[0]);
        return 1;
    }
    
    FILE * f = fopen(argv[1],"r");
    if(!f){
       fprintf(stderr,"failed to open %s\n",argv[1]);
       return 1;
    }
    
    void * prog = malloc(ASM_SIZE);
    void * oaddr = prog;
    if(!prog){
        fclose(f);
        fputs("failed to alloc code buffer!\n",stderr);
        return 1;
    }
    
    int32_t opcount;
    
    uint32_t * patch[10000];
    uint32_t patchcount = 0;
    
    *(char*)oaddr++ = 0x48;
    *(char*)oaddr++ = 0x89;
    *(char*)oaddr++ = 0xf8;
    
    while(1){
        int c = fgetc(f);
        
        /* Use this label for when we already have gotten a char but */
        /* can't process it in the current position. */
        GotChar:
        
        opcount = 0;
        
        if(c == EOF){
            break;
        }
        switch(c) {
            case '+':
                opcount += 2; /* 2 so we can fall through. */
            case '-':
                opcount -= 1;
                while(1){
                    c = fgetc(f);
                    if(c == EOF || c == '>' || c == '<' ||
                                 c == '.' || c == ',' || c == '[' || c == ']'){
                        if(c != ','){ //if we are reading input, we can disregard sums
                            *(char*)oaddr++ = 0x80;
                            *(char*)oaddr++ = 0x00;
                            *(char*)oaddr++ = opcount & 0xff;
                        }
                        goto GotChar;
                    }
                    if(c == '+'){
                        opcount += 1;
                    } else if (c == '-') {
                        opcount -= 1;
                    } /* else we dont care*/
                }
                break;
            case '>':
                opcount += 2; /* 2 so we can fall through. */
            case '<':
                opcount -= 1;
                while(1){
                    c = fgetc(f);
                    if(c == EOF || c == '+' || c == '-' ||
                                 c == '.' || c == ',' || c == '[' || c == ']'){
                        if(opcount >= 0){
                            *(char*)oaddr++ = 0x48;
                            *(char*)oaddr++ = 0x05;
                            *(uint32_t*) oaddr = opcount;
                            oaddr += 4;
                        } else {
                            opcount = -opcount;
                            *(char*)oaddr++ = 0x48;
                            *(char*)oaddr++ = 0x2d;
                            *(uint32_t*) oaddr = opcount;
                            oaddr += 4;
                        }
                        goto GotChar;
                    }
                    if(c == '>'){
                        opcount += 1;
                    } else if (c == '<') {
                        opcount -= 1;
                    } /* else we dont care*/
                }
                break;
            case '[':
                *(char*)oaddr++ = 0x80;
                *(char*)oaddr++ = 0x38;
                *(char*)oaddr++ = 0x00;
                *(char*)oaddr++ = 0x0f;
                *(char*)oaddr++ = 0x84;
                patch[patchcount++] = (uint32_t*)oaddr;
                *(uint32_t*)oaddr = 0;
                oaddr += 4;
                break;
            case ']':
                *(char*)oaddr++ = 0x80;
                *(char*)oaddr++ = 0x38;
                *(char*)oaddr++ = 0x00;
                *(char*)oaddr++ = 0x0f;
                *(char*)oaddr++ = 0x85;
                if(patchcount == 0){
                    fputs("unbalanced braces! fail!\n",stderr);
                    fclose(f);
                    free(prog);
                    return 1;
                }
                patchcount--;
                
                *patch[patchcount] = (uint32_t)((uint64_t)oaddr - (uint64_t)patch[patchcount]);
                *(uint32_t *)oaddr = (uint32_t)((uint64_t)patch[patchcount] - (uint64_t)oaddr);
                
                oaddr += 4;
                break;
            case ',':
                break;
            case '.':
                *(char*)oaddr++ = 0x50;
                *(char*)oaddr++ = 0x8a;
                *(char*)oaddr++ = 0x00;
                *(char*)oaddr++ = 0x89;
                *(char*)oaddr++ = 0xc7;
                *(char*)oaddr++ = 0x48;
                *(char*)oaddr++ = 0xbb;
                *(void **)oaddr = (void*)&putchar;
                oaddr += 8;
                *(char*)oaddr++ = 0xff;
                *(char*)oaddr++ = 0xd3;
                *(char*)oaddr++ = 0x58;
                break;
            default:
                break;
        }
        
    }
    
    *(char*)oaddr = 0xc3;
    oaddr += 1;
    
    fclose(f);
    
    if(c >= 3)  {
        
        FILE * flist = fopen(argv[2],"wb");
        if(!flist) {
            fprintf(stderr,"failed to open the asm dump file %s!\n",argv[2]);
            free(prog);
            return 1;
        }
        
        for(char * p = prog; p != oaddr ; p++){
            fputc(*p,flist);
        }
        
        fclose(flist);
    
    } else {
    
        void * bfArray  = calloc(5000000,1);
        
        if(!bfArray){
            fputs("failed to allocate the brainfuck array!\n",stderr);
            free(prog);
            return 1;
        }
        
        if( mprotect(prog - ((uint64_t)prog % sysconf(_SC_PAGESIZE)),ASM_SIZE,
                                            PROT_READ|PROT_WRITE|PROT_EXEC) ){
            perror("mprotect failed to set memory executable!");
            free(prog);
            free((void*)bfArray);
            return 1;
        }
        ((void (*)(void*))prog)(bfArray+(5000000/2));
        
        //Optimisation is causing bfarray to be freed before use
        //XXX fix and uncomment
        //free(bfArray);
    }
    
    
    free(prog);
    return 0;
}
