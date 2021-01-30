#include "basic.h"
#include "stdlib.h"

int main(void)
{
    system("/bin/stty raw");
    startBasic();
    system("/bin/stty cooked");
    return 0;
}