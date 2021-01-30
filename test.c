#include "stdio.h"
#include "stdlib.h"
#include "basic.h"

int main(void)
{
    NEW();
    printf("Result: %d\n",evalExpression("1*3",0));
    printf("Result: %d\n",evalExpression("A=4/3+2",0));
    printf("Result: %d\n",evalExpression("4/(3+2)",0));
    printf("Result: %d\n",evalExpression("A",0));
    printf("Result: %d\n",evalExpression("A*4",0));
    printf("Result: %d\n",evalExpression("((A>10)||(3/7<3))",0));
    return 0;
}