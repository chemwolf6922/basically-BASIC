#ifndef __IO_H
#define __IO_H

#include "stdio.h"

int getChar(void){
    return getchar();
}

void putChar(int c){
    putchar(c);
    return;
}

#endif